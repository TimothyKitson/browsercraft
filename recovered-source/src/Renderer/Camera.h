#pragma once
#include <glm/glm.hpp>
#include <array>

// Six planes of the view frustum, used to skip chunks that can't be on
// screen. Plane equation: dot(normal, p) + d >= 0 means "inside".
class Frustum
{
public:
    void update(const glm::mat4& viewProjection);
    bool intersectsAABB(const glm::vec3& min, const glm::vec3& max) const;

private:
    std::array<glm::vec4, 6> m_planes{};
};

// Holds position + orientation and builds the view/projection matrices.
// Position is driven by the Player; this class only cares about looking.
class Camera
{
public:
    Camera(glm::vec3 position, float yawDegrees, float pitchDegrees);

    void addLook(float deltaX, float deltaY, float sensitivity);

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;

    glm::vec3 position{ 0.0f };
    glm::vec3 front{ 0.0f, 0.0f, -1.0f };
    glm::vec3 right{ 1.0f, 0.0f, 0.0f };
    glm::vec3 up{ 0.0f, 1.0f, 0.0f };
    static constexpr glm::vec3 WORLD_UP{ 0.0f, 1.0f, 0.0f };

    float yaw;
    float pitch;
    float fov = 70.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

private:
    void updateVectors();
};
