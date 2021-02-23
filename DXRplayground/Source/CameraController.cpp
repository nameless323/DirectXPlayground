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
static constexpr float ScrollMultiplier = 2.0f;
static constexpr float KeyMoveMultiplier = 9.0f;
static constexpr float MouseDeltaMultiplier = 70.0f;

enum KeyboardKeys
{
    KEY_A = 0x41,
    KEY_S = 0x53,
    KEY_D = 0x44,
    KEY_W = 0x57,
    KEY_SPACE = 0x20
};
}

CameraController::CameraController(Camera* camera)
    : m_camera(camera)
{
}

void CameraController::Update()
{
    XMFLOAT3 thisFrameOffset{};

    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    float scroll = io.MouseWheel;
    thisFrameOffset.z = scroll * ScrollMultiplier;

    XMFLOAT2 mouseDelta = { io.MouseDelta.x, io.MouseDelta.y };
    if (io.MouseDown[2])
    {
        thisFrameOffset.x = -mouseDelta.x * KeyMoveMultiplier * dt;
        thisFrameOffset.y = mouseDelta.y * KeyMoveMultiplier * dt;
    }
    thisFrameOffset.x += io.KeysDown[KEY_D] * KeyMoveMultiplier * dt;
    thisFrameOffset.x -= io.KeysDown[KEY_A] * KeyMoveMultiplier * dt;
    thisFrameOffset.z += io.KeysDown[KEY_W] * KeyMoveMultiplier * dt;
    thisFrameOffset.z -= io.KeysDown[KEY_S] * KeyMoveMultiplier * dt;

    float x = 0;
    float y = 0;
    if (!io.MouseDown[2] && io.MouseDown[1])
    {
        x = XMConvertToRadians(mouseDelta.x) * MouseDeltaMultiplier * dt;
        y = XMConvertToRadians(mouseDelta.y) * MouseDeltaMultiplier * dt;
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

}