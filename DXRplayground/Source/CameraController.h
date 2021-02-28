#pragma once

namespace DirectX
{
struct XMFLOAT3;
}

namespace DirectxPlayground
{
class Camera;

class CameraController
{
public:
    CameraController(Camera* camera);
    void Update();

private:
    void UpdateCameraMatrices(float xRotation, float yRotation, const DirectX::XMFLOAT3& localOffsets);
    void DrawImgui();

    Camera* m_camera = nullptr;
    float m_scrollMultiplier = 2.0f;
    float m_keyMoveMultiplier = 9.0f;
    float m_mouseDeltaMultiplier = 70.0f;
    float m_shiftMoveMultiplier = 4.0f;
    float m_ctrlMoveMultiplier = 0.25f;
};
}
