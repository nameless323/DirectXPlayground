#pragma once

namespace DirectxPlayground
{
class Camera;

class CameraController
{
public:
    CameraController(Camera* camera);
    void Update();

private:
    Camera* m_camera = nullptr;
};
}
