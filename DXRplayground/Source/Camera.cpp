#include "Camera.h"

namespace DirectxPlayground
{

Camera::Camera(float fovY, float aspect, float nearZ, float farZ)
    : m_dirty(false)
{
    XMMATRIX view = XMMatrixIdentity();
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    XMMATRIX vp = view * proj;
    XMStoreFloat4x4(&m_projection, proj);
    XMStoreFloat4x4(&m_view, view);
    XMStoreFloat4x4(&m_toWorld, view);
    XMStoreFloat4x4(&m_viewProjection, vp);
}

void Camera::SetWorldPosition(const XMFLOAT3& pos, bool updateViewAndVP /* = true*/)
{
    m_toWorld(3, 0) = pos.x;
    m_toWorld(3, 1) = pos.y;
    m_toWorld(3, 2) = pos.z;
    if (updateViewAndVP)
    {
        XMMATRIX cameraToWorld = XMLoadFloat4x4(&m_toWorld);
        XMMATRIX newView = XMMatrixInverse(&XMMatrixDeterminant(cameraToWorld), cameraToWorld);
        XMFLOAT4X4 newViewStored;
        XMStoreFloat4x4(&newViewStored, newView);
        SetView(newViewStored, m_toWorld, true);
    }
}

}