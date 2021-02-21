#include "CameraController.h"

#include "Camera.h"

#include <DirectXMath.h>
#include <Windows.h>

#include "External/IMGUI/imgui.h"

namespace DirectxPlayground
{
enum KeyboardKeys
{
    KEY_A = 0x41,
    KEY_S = 0x52,
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
    XMFLOAT4X4 view = {};
    XMStoreFloat4x4(&view, viewSimd);
    XMVECTOR right = { view(0, 0), view(1, 0), view(2, 0) };
    XMVECTOR up = { view(0, 1), view(1, 1), view(2, 1) };
    XMVECTOR fwd = { view(0, 2), view(1, 2), view(2, 2) };
    XMVECTOR pos = { view(3, 0), view(3, 1), view(3, 2) };

    XMFLOAT3 thisFrameOffset{};

    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;
    if (io.KeysDown[KEY_A])
        OutputDebugStringA("A pressed\n");

    float scroll = io.MouseWheel;
    thisFrameOffset.z = scroll;

    XMFLOAT2 mouseDelta = { io.MouseDelta.x, io.MouseDelta.y };
    if (io.MouseDown[2])
    {
        thisFrameOffset.x = mouseDelta.x;
        thisFrameOffset.y = mouseDelta.y;
    }
    thisFrameOffset.x += io.KeysDown[KEY_D];
    thisFrameOffset.y -= io.KeysDown[KEY_D];

    float x = 0;
    float y = 0;
    if (!io.MouseDown[2] && io.MouseDown[1])
    {
        x = XMConvertToRadians(-mouseDelta.x);
        y = XMConvertToRadians(-mouseDelta.y);
    }
    XMMATRIX pitch = XMMatrixRotationAxis(right, y);
    XMMATRIX yaw = XMMatrixRotationY(x);
    XMMATRIX frameRot = pitch * yaw;
}

}