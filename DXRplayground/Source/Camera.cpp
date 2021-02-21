#include "Camera.h"

namespace DirectxPlayground
{

Camera::Camera(float fovY, float aspect, float nearZ, float farZ)
    : m_dirty(false)
{
    m_projection = XMMatrixPerspectiveFovLH(fovY, aspect, nearZ, farZ);
    m_view = XMMatrixIdentity();
    m_viewProjection = m_view * m_projection;
}

}