#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

void Frustum::update(const glm::mat4& m)
{
    // Gribb/Hartmann plane extraction from the combined view-projection.
    m_planes[0] = glm::vec4(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]); // left
    m_planes[1] = glm::vec4(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]); // right
    m_planes[2] = glm::vec4(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]); // bottom
    m_planes[3] = glm::vec4(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]); // top
    m_planes[4] = glm::vec4(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]); // near
    m_planes[5] = glm::vec4(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]); // far

    for (auto& plane : m_planes)
    {
        float length = glm::length(glm::vec3(plane));
        if (length > 0.0f) plane /= length;
    }
}

bool Frustum::intersectsAABB(const glm::vec3& min, const glm::vec3& max) const
{
    for (const auto& plane : m_planes)
    {
        // Test the box corner furthest along the plane normal: if even that
        // one is behind the plane, the whole box is outside.
        glm::vec3 positive(plane.x >= 0.0f ? max.x : min.x,
                           plane.y >= 0.0f ? max.y : min.y,
                           plane.z >= 0.0f ? max.z : min.z);
        if (glm::dot(glm::vec3(plane), positive) + plane.w < 0.0f)
            return false;
    }
    return true;
}

Camera::Camera(glm::vec3 startPosition, float yawDegrees, float pitchDegrees)
    : position(startPosition), yaw(yawDegrees), pitch(pitchDegrees)
{
    updateVectors();
}

void Camera::addLook(float deltaX, float deltaY, float sensitivity)
{
    yaw += deltaX * sensitivity;
    pitch -= deltaY * sensitivity;
    pitch = std::clamp(pitch, -89.9f, 89.9f);
    if (yaw > 360.0f) yaw -= 360.0f;
    if (yaw < -360.0f) yaw += 360.0f;
    updateVectors();
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::projectionMatrix(float aspect) const
{
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::updateVectors()
{
    glm::vec3 f;
    f.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    f.y = std::sin(glm::radians(pitch));
    f.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, worldUp()));
    up = glm::normalize(glm::cross(right, front));
}
