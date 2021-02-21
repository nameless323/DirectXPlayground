#include "CameraController.h"

#include "Camera.h"

#include <DirectXMath.h>
#include <Windows.h>

#include "External/IMGUI/imgui.h"

#include <sstream>

namespace DirectxPlayground
{
enum KeyboardKeys
{
    KEY_A = 0x41,
    KEY_S = 0x53,
    KEY_D = 0x44,
    KEY_W = 0x57,
    KEY_SPACE = 0x20
};

CameraController::CameraController(Camera* camera)
    : m_camera(camera)
{
}

void CameraController::Update()
{
    XMMATRIX viewSimd = m_camera->GetView();
    XMMATRIX cameraToWorld = XMMatrixInverse(&XMMatrixDeterminant(viewSimd), viewSimd);
    XMFLOAT4X4 view = {};
    XMStoreFloat4x4(&view, viewSimd);
    XMVECTOR right = { view(0, 0), view(1, 0), view(2, 0) };
    XMVECTOR up = { view(0, 1), view(1, 1), view(2, 1) };
    XMVECTOR fwd = { view(0, 2), view(1, 2), view(2, 2) };
    XMVECTOR pos = { view(3, 0), view(3, 1), view(3, 2) };

    XMFLOAT3 thisFrameOffset{};

    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    float scroll = io.MouseWheel;
    thisFrameOffset.z = scroll * 0.01f;

    XMFLOAT2 mouseDelta = { io.MouseDelta.x, io.MouseDelta.y };
    if (io.MouseDown[2])
    {
        thisFrameOffset.x = -mouseDelta.x * 0.005f;
        thisFrameOffset.y = mouseDelta.y * 0.005f;
    }
    thisFrameOffset.x += io.KeysDown[KEY_D] * 0.005f;
    thisFrameOffset.x -= io.KeysDown[KEY_A] * 0.005f;
    thisFrameOffset.z += io.KeysDown[KEY_W] * 0.005f;
    thisFrameOffset.z -= io.KeysDown[KEY_S] * 0.005f;

    float x = 0;
    float y = 0;
    if (!io.MouseDown[2] && io.MouseDown[1])
    {
        x = XMConvertToRadians(mouseDelta.x) * 0.1f;
        y = XMConvertToRadians(mouseDelta.y) * 0.1f;
    }
    XMMATRIX pitch = XMMatrixRotationAxis(right, y);
    XMMATRIX yaw = XMMatrixRotationY(x);
    XMMATRIX frameRot = pitch * yaw;

    XMVECTOR offset = XMVector4Transform(XMVECTOR{ thisFrameOffset.x, thisFrameOffset.y, thisFrameOffset.z, 1.0f }, cameraToWorld);
    XMFLOAT4 storedOffset;
    XMStoreFloat4(&storedOffset, offset);

    cameraToWorld *= frameRot;
    XMFLOAT4X4 storedInv;
    XMStoreFloat4x4(&storedInv, cameraToWorld);

    storedInv(3, 0) = storedOffset.x;
    storedInv(3, 1) = storedOffset.y;
    storedInv(3, 2) = storedOffset.z;
    cameraToWorld = XMLoadFloat4x4(&storedInv);
    m_camera->SetView(XMMatrixInverse(&XMMatrixDeterminant(cameraToWorld), cameraToWorld), true);
}

}