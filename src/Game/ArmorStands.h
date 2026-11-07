#pragma once
#include "Player/Inventory.h"
#include <glm/glm.hpp>
#include <array>
#include <cstdint>
#include <vector>

namespace ArmorStands
{
    enum Slot { Head, Chest, Legs, Feet, MainHand, OffHand, SLOTS };

    constexpr float WIDTH = 0.5f;
    constexpr float HEIGHT = 1.975f;
    constexpr float HIT_WINDOW = 0.25f;

    struct Angles
    {
        glm::vec3 head{ 0.0f };
        glm::vec3 body{ 0.0f };
        glm::vec3 leftArm{ 0.0f };
        glm::vec3 rightArm{ 0.0f };
        glm::vec3 leftLeg{ 0.0f };
        glm::vec3 rightLeg{ 0.0f };
    };

    struct Pose
    {
        const char* name;
        Angles angles;
    };
    const std::vector<Pose>& poses();

    struct Stand
    {
        glm::vec3 feet{ 0.0f };
        float yaw = 0.0f;
        int pose = 0;
        std::array<ItemStack, SLOTS> gear{};
        float velocityY = 0.0f;
        float sinceHit = 10.0f;
    };

    Slot preferredSlot(StackId item);
    Slot slotAtHeight(float height, const Stand& stand);
    float placedYaw(float lookYaw);

    struct Outcome
    {
        bool changed = false;
        ItemStack hand;
    };
    Outcome use(Stand& stand, const ItemStack& held, float hitHeight, bool creative);

    bool hit(Stand& stand, bool creative);

    bool rayHits(const Stand& stand, const glm::vec3& origin, const glm::vec3& direction, float reach,
                 float& distance, float& height);

    glm::vec3 rotated(const glm::vec3& point, const glm::vec3& degrees);
    glm::vec3 toWorld(const Stand& stand, const glm::vec3& model);
    glm::vec3 handPosition(const Stand& stand, bool right);
    glm::vec3 headCentre(const Stand& stand);
}

class ArmorStandStore
{
public:
    void setDimension(int dimension) { m_dimension = dimension; }
    void clear() { m_stands.clear(); }

    struct Entry
    {
        int dimension = 0;
        ArmorStands::Stand stand;
    };

    void add(const ArmorStands::Stand& stand) { m_stands.push_back(Entry{ m_dimension, stand }); }
    void addIn(int dimension, const ArmorStands::Stand& stand) { m_stands.push_back(Entry{ dimension, stand }); }
    std::vector<Entry>& all() { return m_stands; }
    const std::vector<Entry>& all() const { return m_stands; }
    int dimension() const { return m_dimension; }

private:
    int m_dimension = 0;
    std::vector<Entry> m_stands;
};
