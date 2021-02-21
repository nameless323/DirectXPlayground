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

}