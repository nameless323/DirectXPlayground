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
    CameraController(Camera* camera, float scrollMult = 1.0f, float keyMoveMult = 3.0f, float mouseDeltaMult = 30.0f, float shiftMoveMult = 4.0f, float ctrlMoveMult = 0.25f);
    void Update();

private:
    void UpdateCameraMatrices(float xRotation, float yRotation, const DirectX::XMFLOAT3& localOffsets);
    void DrawImgui();

    Camera* m_camera = nullptr;
    float m_scrollMultiplier = 1.0f;
    float m_keyMoveMultiplier = 3.0f;
    float m_mouseDeltaMultiplier = 30.0f;
    float m_shiftMoveMultiplier = 4.0f;
    float m_ctrlMoveMultiplier = 0.25f;
};
}
