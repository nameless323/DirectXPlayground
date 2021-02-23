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

    Camera* m_camera = nullptr;
};
}
