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

    Camera* mCamera = nullptr;
    float mScrollMultiplier = 1.0f;
    float mKeyMoveMultiplier = 3.0f;
    float mMouseDeltaMultiplier = 30.0f;
    float mShiftMoveMultiplier = 4.0f;
    float mCtrlMoveMultiplier = 0.25f;
};
}
