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

CameraController::CameraController(Camera* camera)
    : m_camera(camera)
{
}

void CameraController::Update()
{
    DrawImgui();
    XMFLOAT3 thisFrameOffset{};

    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    float scroll = io.MouseWheel;
    thisFrameOffset.z = scroll * m_scrollMultiplier;

    XMFLOAT2 mouseDelta = { io.MouseDelta.x, io.MouseDelta.y };
    if (io.MouseDown[2])
    {
        thisFrameOffset.x = -mouseDelta.x * m_keyMoveMultiplier * dt;
        thisFrameOffset.y = mouseDelta.y * m_keyMoveMultiplier * dt;
    }
    thisFrameOffset.x += io.KeysDown[KEY_D] * m_keyMoveMultiplier * dt;
    thisFrameOffset.x -= io.KeysDown[KEY_A] * m_keyMoveMultiplier * dt;
    thisFrameOffset.z += io.KeysDown[KEY_W] * m_keyMoveMultiplier * dt;
    thisFrameOffset.z -= io.KeysDown[KEY_S] * m_keyMoveMultiplier * dt;

    if (io.KeysDown[KEY_SHIFT])
    {
        thisFrameOffset.x *= m_shiftMoveMultiplier;
        thisFrameOffset.y *= m_shiftMoveMultiplier;
        thisFrameOffset.z *= m_shiftMoveMultiplier;
    }
    if (io.KeysDown[KEY_CTRL])
    {
        thisFrameOffset.x *= m_ctrlMoveMultiplier;
        thisFrameOffset.y *= m_ctrlMoveMultiplier;
        thisFrameOffset.z *= m_ctrlMoveMultiplier;
    }

    float x = 0;
    float y = 0;
    if (!io.MouseDown[2] && io.MouseDown[1])
    {
        x = XMConvertToRadians(mouseDelta.x) * m_mouseDeltaMultiplier * dt;
        y = XMConvertToRadians(mouseDelta.y) * m_mouseDeltaMultiplier * dt;
    }

    UpdateCameraMatrices(x, y, thisFrameOffset);

}

void CameraController::UpdateCameraMatrices(float xRotation, float yRotation, const XMFLOAT3& localOffsets)
{
    XMVECTOR camRight = XMLoadFloat4(&m_camera->GetRight());

    XMMATRIX pitch = XMMatrixRotationAxis(camRight, yRotation);
    XMMATRIX yaw = XMMatrixRotationY(xRotation);
    XMMATRIX frameRot = pitch * yaw;

    XMMATRIX cameraToWorld = XMLoadFloat4x4(&m_camera->GetToWorld());
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
    XMMATRIX newView = XMMatrixInverse(&XMMatrixDeterminant(cameraToWorld), cameraToWorld);
    XMFLOAT4X4 newViewStored;
    XMStoreFloat4x4(&newViewStored, newView);

    m_camera->SetView(newViewStored, storedToWorld, true);
}

void CameraController::DrawImgui()
{
    ImGui::Begin("Camera Controls");
    ImGui::InputFloat("Scroll Multiplier", &m_scrollMultiplier);
    ImGui::InputFloat("Keys Move Multiplier", &m_keyMoveMultiplier);
    ImGui::InputFloat("Mouse Delta Multiplier", &m_mouseDeltaMultiplier);
    ImGui::InputFloat("Shift Move Multiplier", &m_shiftMoveMultiplier);
    ImGui::InputFloat("Ctrl Move Multiplier", &m_ctrlMoveMultiplier);
    ImGui::End();
}

}