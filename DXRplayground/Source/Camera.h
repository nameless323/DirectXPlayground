#pragma once

#include <DirectXMath.h>

namespace DirectxPlayground
{
using namespace DirectX;

class Camera
{
public:
    Camera(float fovY, float aspect, float nearZ, float farZ);

    const XMMATRIX& GetView() const;
    const XMMATRIX& GetProjection() const;
    const XMMATRIX& GetViewProjection() const;

    void UpdateViewProjection();
    void SetView(FXMMATRIX view, bool updateViewProjection = false);

private:
    XMMATRIX m_projection;
    XMMATRIX m_view;
    XMMATRIX m_viewProjection;
    bool m_dirty;
};

inline const XMMATRIX& Camera::GetView() const
{
    return m_view;
}

inline const XMMATRIX& Camera::GetProjection() const
{
    return m_projection;
}

inline const XMMATRIX& Camera::GetViewProjection() const
{
    return m_viewProjection;
}

inline void Camera::UpdateViewProjection()
{
    if (!m_dirty)
        return;
    m_viewProjection = m_view * m_projection;
    m_dirty = false;
}

inline void Camera::SetView(FXMMATRIX view, bool updateViewProjection /* = false */)
{
    m_view = view;
    m_dirty = true;
    if (updateViewProjection)
        UpdateViewProjection();
}
}

