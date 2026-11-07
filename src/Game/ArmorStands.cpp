#include "ArmorStands.h"
#include "Armour.h"
#include "Items.h"
#include "World/Block.h"
#include <algorithm>
#include <cmath>

namespace ArmorStands
{
    const std::vector<Pose>& poses()
    {
        static const std::vector<Pose> LIST = {
            { "Default", { {}, {}, { -10.0f, 0.0f, -10.0f }, { -15.0f, 0.0f, 10.0f }, { -1.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 1.0f } } },
            { "No Pose", { {}, {}, {}, {}, {}, {} } },
            { "Solemn", { { 15.0f, 0.0f, 0.0f }, {}, { -30.0f, 15.0f, 15.0f }, { -30.0f, -15.0f, -15.0f }, {}, {} } },
            { "Athena", { { -5.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 2.0f }, { 10.0f, 0.0f, -5.0f }, { -60.0f, 20.0f, -10.0f },
                          { -3.0f, -3.0f, -3.0f }, { 3.0f, 3.0f, 3.0f } } },
            { "Brandish", { { -15.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -2.0f }, { 20.0f, 0.0f, -10.0f }, { -110.0f, 50.0f, 0.0f },
                            { 5.0f, -3.0f, -3.0f }, { -5.0f, 3.0f, 3.0f } } },
            { "Honor", { { -15.0f, 0.0f, 0.0f }, {}, { -110.0f, 35.0f, 0.0f }, { -110.0f, -35.0f, 0.0f },
                         { 5.0f, -3.0f, -3.0f }, { -5.0f, 3.0f, 3.0f } } },
            { "Entertain", { { -15.0f, 0.0f, 0.0f }, {}, { -110.0f, -35.0f, 0.0f }, { -110.0f, 35.0f, 0.0f },
                             { 5.0f, -3.0f, -3.0f }, { -5.0f, 3.0f, 3.0f } } },
            { "Salute", { {}, {}, { 10.0f, 0.0f, -5.0f }, { -70.0f, -40.0f, 0.0f }, { -3.0f, -3.0f, -3.0f }, { 3.0f, 3.0f, 3.0f } } },
            { "Zombie", { { -10.0f, 0.0f, -5.0f }, {}, { -105.0f, 0.0f, 0.0f }, { -100.0f, 0.0f, 0.0f },
                          { 7.0f, 0.0f, 0.0f }, { -46.0f, 0.0f, 0.0f } } },
            { "Cancan A", { { -5.0f, 18.0f, 0.0f }, { 0.0f, 22.0f, 0.0f }, { 8.0f, 0.0f, -114.0f }, { 0.0f, 84.0f, 111.0f },
                            { -111.0f, 55.0f, 0.0f }, { 0.0f, 23.0f, -13.0f } } },
            { "Cancan B", { { -10.0f, -20.0f, 0.0f }, { 0.0f, -18.0f, 0.0f }, { 0.0f, 0.0f, -112.0f }, { 8.0f, 90.0f, 111.0f },
                            { 0.0f, 0.0f, 13.0f }, { -119.0f, -42.0f, 0.0f } } },
            { "Hero", { { -4.0f, 67.0f, 0.0f }, { 0.0f, 8.0f, 0.0f }, { 16.0f, 32.0f, -8.0f }, { -99.0f, 63.0f, 0.0f },
                        { 0.0f, -75.0f, -8.0f }, { 4.0f, 63.0f, 8.0f } } },
        };
        return LIST;
    }

    Slot preferredSlot(StackId item)
    {
        if (item == Items::Elytra) return Chest;
        switch (Armour::pieceOf(item))
        {
            case Armour::Piece::Helmet: return Head;
            case Armour::Piece::Chestplate: return Chest;
            case Armour::Piece::Leggings: return Legs;
            case Armour::Piece::Boots: return Feet;
            default: break;
        }
        if (!isItem(item) && isCarvedPumpkin(asBlock(item))) return Head;
        return MainHand;
    }

    Slot slotAtHeight(float height, const Stand& stand)
    {
        const double d = height;
        auto has = [&](Slot s) { return !stand.gear[s].empty(); };
        if (d >= 0.1 && d < 0.55 && has(Feet)) return Feet;
        if (d >= 0.9 && d < 1.6 && has(Chest)) return Chest;
        if (d >= 0.4 && d < 1.2 && has(Legs)) return Legs;
        if (d >= 1.6 && has(Head)) return Head;
        if (!has(MainHand) && has(OffHand)) return OffHand;
        return MainHand;
    }

    float placedYaw(float lookYaw)
    {
        float facing = lookYaw + 180.0f;
        facing = std::floor((facing + 22.5f) / 45.0f) * 45.0f;
        facing = std::fmod(facing, 360.0f);
        if (facing < 0.0f) facing += 360.0f;
        return facing;
    }

    Outcome use(Stand& stand, const ItemStack& held, float hitHeight, bool creative)
    {
        Outcome out;
        out.hand = held;
        if (held.empty())
        {
            const Slot slot = slotAtHeight(hitHeight, stand);
            if (stand.gear[slot].empty()) return out;
            out.hand = stand.gear[slot];
            stand.gear[slot].clear();
            out.changed = true;
            return out;
        }

        const Slot slot = preferredSlot(held.id);
        ItemStack& worn = stand.gear[slot];
        ItemStack one = held;
        one.count = 1;
        if (creative && worn.empty())
        {
            worn = one;
            out.changed = true;
            return out;
        }
        if (held.count <= 1)
        {
            out.hand = worn;
            worn = held;
            out.changed = true;
            return out;
        }
        if (!worn.empty()) return out;
        worn = one;
        out.hand.count -= 1;
        out.changed = true;
        return out;
    }

    bool hit(Stand& stand, bool creative)
    {
        if (creative) return true;
        if (stand.sinceHit <= HIT_WINDOW) return true;
        stand.sinceHit = 0.0f;
        return false;
    }

    glm::vec3 rotated(const glm::vec3& point, const glm::vec3& degrees)
    {
        const glm::vec3 a = glm::radians(degrees);
        glm::vec3 p = point;
        float s = std::sin(a.x), c = std::cos(a.x);
        p = glm::vec3(p.x, p.y * c - p.z * s, p.y * s + p.z * c);
        s = std::sin(a.y); c = std::cos(a.y);
        p = glm::vec3(p.x * c + p.z * s, p.y, -p.x * s + p.z * c);
        s = std::sin(a.z); c = std::cos(a.z);
        p = glm::vec3(p.x * c - p.y * s, p.x * s + p.y * c, p.z);
        return p;
    }

    glm::vec3 toWorld(const Stand& stand, const glm::vec3& model)
    {
        const float yaw = glm::radians(stand.yaw);
        const glm::vec3 forward(std::cos(yaw), 0.0f, std::sin(yaw));
        const glm::vec3 right(-forward.z, 0.0f, forward.x);
        return stand.feet + (right * model.x + glm::vec3(0.0f, model.y, 0.0f) + forward * model.z) / 16.0f;
    }

    glm::vec3 handPosition(const Stand& stand, bool right)
    {
        const Angles& angles = poses()[static_cast<size_t>(std::clamp(stand.pose, 0, static_cast<int>(poses().size()) - 1))].angles;
        const glm::vec3 pivot = right ? glm::vec3(5.0f, 22.0f, 0.0f) : glm::vec3(-5.0f, 22.0f, 0.0f);
        const glm::vec3 tip = right ? glm::vec3(1.0f, -10.0f, 1.5f) : glm::vec3(-1.0f, -10.0f, 1.5f);
        return toWorld(stand, pivot + rotated(tip, right ? angles.rightArm : angles.leftArm));
    }

    glm::vec3 headCentre(const Stand& stand)
    {
        const Angles& angles = poses()[static_cast<size_t>(std::clamp(stand.pose, 0, static_cast<int>(poses().size()) - 1))].angles;
        return toWorld(stand, glm::vec3(0.0f, 23.0f, 0.0f) + rotated(glm::vec3(0.0f, 4.0f, 0.0f), angles.head));
    }

    bool rayHits(const Stand& stand, const glm::vec3& origin, const glm::vec3& direction, float reach,
                 float& distance, float& height)
    {
        const glm::vec3 min = stand.feet - glm::vec3(WIDTH * 0.5f, 0.0f, WIDTH * 0.5f);
        const glm::vec3 max = stand.feet + glm::vec3(WIDTH * 0.5f, HEIGHT, WIDTH * 0.5f);
        float enter = 0.0f, leave = reach;
        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::fabs(direction[axis]) < 1e-6f)
            {
                if (origin[axis] < min[axis] || origin[axis] > max[axis]) return false;
                continue;
            }
            float a = (min[axis] - origin[axis]) / direction[axis];
            float b = (max[axis] - origin[axis]) / direction[axis];
            if (a > b) std::swap(a, b);
            enter = std::max(enter, a);
            leave = std::min(leave, b);
            if (enter > leave) return false;
        }
        distance = enter;
        height = origin.y + direction.y * enter - stand.feet.y;
        return true;
    }
}
