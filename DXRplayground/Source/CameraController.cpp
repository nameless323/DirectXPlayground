#include "CameraController.h"

#include "Camera.h"

#include <DirectXMath.h>
#include <Windows.h>

#include "External/IMGUI/imgui.h"

#include <sstream>

namespace DirectxPlayground
{
namespace
{
enum KeyboardKeys
{
    KEY_A = 0x41,
    KEY_S = 0x53,
    KEY_D = 0x44,
    KEY_W = 0x57,
    KEY_SPACE = 0x20,
    KEY_SHIFT = 0x10,
    KEY_CTRL = 0x11
};
}

CameraController::CameraController(Camera* camera, float scrollMult /*= 1.0f*/, float keyMoveMult /*= 3.0f*/, float mouseDeltaMult /*= 30.0f*/, float shiftMoveMult /*= 4.0f*/, float ctrlMoveMult /*= 0.25f*/)
    : mCamera(camera)
    , mScrollMultiplier(scrollMult)
    , mKeyMoveMultiplier(keyMoveMult)
    , mMouseDeltaMultiplier(mouseDeltaMult)
    , mShiftMoveMultiplier(shiftMoveMult)
    , mCtrlMoveMultiplier(ctrlMoveMult)
{
}

void CameraController::Update()
{
    DrawImgui();
    XMFLOAT3 thisFrameOffset{};

    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    float scroll = io.MouseWheel;
    thisFrameOffset.z = scroll * mScrollMultiplier;

    XMFLOAT2 mouseDelta = { io.MouseDelta.x, io.MouseDelta.y };
    if (io.MouseDown[2])
    {
        thisFrameOffset.x = -mouseDelta.x * mKeyMoveMultiplier * dt;
        thisFrameOffset.y = mouseDelta.y * mKeyMoveMultiplier * dt;
    }
    thisFrameOffset.x += io.KeysDown[KEY_D] * mKeyMoveMultiplier * dt;
    thisFrameOffset.x -= io.KeysDown[KEY_A] * mKeyMoveMultiplier * dt;
    thisFrameOffset.z += io.KeysDown[KEY_W] * mKeyMoveMultiplier * dt;
    thisFrameOffset.z -= io.KeysDown[KEY_S] * mKeyMoveMultiplier * dt;

    if (io.KeysDown[KEY_SHIFT])
    {
        thisFrameOffset.x *= mShiftMoveMultiplier;
        thisFrameOffset.y *= mShiftMoveMultiplier;
        thisFrameOffset.z *= mShiftMoveMultiplier;
    }
    if (io.KeysDown[KEY_CTRL])
    {
        thisFrameOffset.x *= mCtrlMoveMultiplier;
        thisFrameOffset.y *= mCtrlMoveMultiplier;
        thisFrameOffset.z *= mCtrlMoveMultiplier;
    }

    float x = 0;
    float y = 0;
    if (!io.MouseDown[2] && io.MouseDown[1])
    {
        x = XMConvertToRadians(mouseDelta.x) * mMouseDeltaMultiplier * dt;
        y = XMConvertToRadians(mouseDelta.y) * mMouseDeltaMultiplier * dt;
    }

    UpdateCameraMatrices(x, y, thisFrameOffset);

}

void CameraController::UpdateCameraMatrices(float xRotation, float yRotation, const XMFLOAT3& localOffsets)
{
    XMFLOAT4 camRightV = mCamera->GetRight();
    XMVECTOR camRight = XMLoadFloat4(&camRightV);

    XMMATRIX pitch = XMMatrixRotationAxis(camRight, yRotation);
    XMMATRIX yaw = XMMatrixRotationY(xRotation);
    XMMATRIX frameRot = pitch * yaw;

    XMMATRIX cameraToWorld = XMLoadFloat4x4(&mCamera->GetToWorld());
    XMVECTOR offset = XMVector4Transform(XMVECTOR{ localOffsets.x, localOffsets.y, localOffsets.z, 1.0f }, cameraToWorld);
    XMFLOAT4 storedOffset;
    XMStoreFloat4(&storedOffset, offset);

    cameraToWorld *= frameRot;
    XMFLOAT4X4 storedToWorld;
    XMStoreFloat4x4(&storedToWorld, cameraToWorld);

    storedToWorld(3, 0) = storedOffset.x;
    storedToWorld(3, 1) = storedOffset.y;
    storedToWorld(3, 2) = storedOffset.z;
    cameraToWorld = XMLoadFloat4x4(&storedToWorld);
    XMVECTOR det = XMMatrixDeterminant(cameraToWorld);
    XMMATRIX newView = XMMatrixInverse(&det, cameraToWorld);
    XMFLOAT4X4 newViewStored;
    XMStoreFloat4x4(&newViewStored, newView);

    mCamera->SetView(newViewStored, storedToWorld, true);
}

void CameraController::DrawImgui()
{
    ImGui::Begin("Camera Controls");
    ImGui::InputFloat("Scroll Multiplier", &mScrollMultiplier);
    ImGui::InputFloat("Keys Move Multiplier", &mKeyMoveMultiplier);
    ImGui::InputFloat("Mouse Delta Multiplier", &mMouseDeltaMultiplier);
    ImGui::InputFloat("Shift Move Multiplier", &mShiftMoveMultiplier);
    ImGui::InputFloat("Ctrl Move Multiplier", &mCtrlMoveMultiplier);
    ImGui::End();
}

}