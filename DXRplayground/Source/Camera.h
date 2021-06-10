#pragma once

#include <DirectXMath.h>

namespace DirectxPlayground
{
using namespace DirectX;

struct CameraShaderData
{
    XMFLOAT4X4 ViewProj;
    XMFLOAT3 Position;
    float Padding;
};

class Camera
{
public:
    Camera(float fovY, float aspect, float nearZ, float farZ);

    const XMFLOAT4X4& GetView() const;
    const XMFLOAT4X4& GetProjection() const;
    const XMFLOAT4X4& GetViewProjection() const;
    const XMFLOAT4X4& GetToWorld() const;

    XMFLOAT4 GetForward() const;
    XMFLOAT4 GetRight() const;
    XMFLOAT4 GetUp() const;
    XMFLOAT4 GetPosition() const;

    void UpdateViewProjection();
    void SetView(const XMFLOAT4X4& view, const XMFLOAT4X4& toWorld, bool updateViewProjection = false);
    void SetWorldPosition(const XMFLOAT3& pos, bool updateViewAndVP = true);

private:
    XMFLOAT4X4 m_projection;
    XMFLOAT4X4 m_view;
    XMFLOAT4X4 m_toWorld;
    XMFLOAT4X4 m_viewProjection;
    bool m_dirty;
};

inline const XMFLOAT4X4& Camera::GetView() const
{
    return m_view;
}

inline const XMFLOAT4X4& Camera::GetProjection() const
{
    return m_projection;
}

inline const XMFLOAT4X4& Camera::GetViewProjection() const
{
    return m_viewProjection;
}

inline const XMFLOAT4X4& Camera::GetToWorld() const
{
    return m_toWorld;
}

inline XMFLOAT4 Camera::GetForward() const
{
    return XMFLOAT4{ m_view(0, 2), m_view(1, 2), m_view(2, 2), 0.0f };
}

inline XMFLOAT4 Camera::GetRight() const
{
    return XMFLOAT4{ m_view(0, 0), m_view(1, 0), m_view(2, 0), 0.0f };
}

inline XMFLOAT4 Camera::GetUp() const
{
    return XMFLOAT4{ m_view(0, 1), m_view(1, 1), m_view(2, 1), 0.0f };
}

inline XMFLOAT4 Camera::GetPosition() const
{
    return XMFLOAT4{ m_toWorld(3, 0), m_toWorld(3, 1), m_toWorld(3, 2), 1.0f };
}

inline void Camera::UpdateViewProjection()
{
    if (!m_dirty)
        return;
    XMMATRIX view = XMLoadFloat4x4(&m_view);
    XMMATRIX proj = XMLoadFloat4x4(&m_projection);
    XMStoreFloat4x4(&m_viewProjection, view * proj);
    m_dirty = false;
}

inline void Camera::SetView(const XMFLOAT4X4& view, const XMFLOAT4X4& toWorld, bool updateViewProjection /* = false */)
{
    m_view = view;
    m_toWorld = toWorld;
    m_dirty = true;
    if (updateViewProjection)
        UpdateViewProjection();
}
}

