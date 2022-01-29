#pragma once

#include <DirectXMath.h>

namespace DirectxPlayground
{
using namespace DirectX;

struct CameraShaderData
{
    XMFLOAT4X4 ViewProj;
    XMFLOAT4X4 View;
    XMFLOAT4X4 Proj;
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
    XMFLOAT4X4 mProjection;
    XMFLOAT4X4 mView;
    XMFLOAT4X4 mToWorld;
    XMFLOAT4X4 mViewProjection;
    bool mDirty;
};

inline const XMFLOAT4X4& Camera::GetView() const
{
    return mView;
}

inline const XMFLOAT4X4& Camera::GetProjection() const
{
    return mProjection;
}

inline const XMFLOAT4X4& Camera::GetViewProjection() const
{
    return mViewProjection;
}

inline const XMFLOAT4X4& Camera::GetToWorld() const
{
    return mToWorld;
}

inline XMFLOAT4 Camera::GetForward() const
{
    return XMFLOAT4{ mView(0, 2), mView(1, 2), mView(2, 2), 0.0f };
}

inline XMFLOAT4 Camera::GetRight() const
{
    return XMFLOAT4{ mView(0, 0), mView(1, 0), mView(2, 0), 0.0f };
}

inline XMFLOAT4 Camera::GetUp() const
{
    return XMFLOAT4{ mView(0, 1), mView(1, 1), mView(2, 1), 0.0f };
}

inline XMFLOAT4 Camera::GetPosition() const
{
    return XMFLOAT4{ mToWorld(3, 0), mToWorld(3, 1), mToWorld(3, 2), 1.0f };
}

inline void Camera::UpdateViewProjection()
{
    if (!mDirty)
        return;
    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProjection);
    XMStoreFloat4x4(&mViewProjection, view * proj);
    mDirty = false;
}

inline void Camera::SetView(const XMFLOAT4X4& view, const XMFLOAT4X4& toWorld, bool updateViewProjection /* = false */)
{
    mView = view;
    mToWorld = toWorld;
    mDirty = true;
    if (updateViewProjection)
        UpdateViewProjection();
}
}

