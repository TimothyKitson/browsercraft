#include "MobType.h"
#include <algorithm>

namespace
{
    constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
    {
        return 0xFF000000u | (static_cast<uint32_t>(b) << 16) |
               (static_cast<uint32_t>(g) << 8) | r;
    }

    MobBox box(Part part, int w, int h, int d, int x, int y, int z)
    {
        MobBox b;
        b.part = part;
        b.size = glm::ivec3(w, h, d);
        b.origin = glm::ivec3(x, y, z);
        return b;
    }

    // Four legs at the corners of a body, with the front pair at +z.
    void addLegs(std::vector<MobBox>& out, int legW, int legH, int legD,
                 int spreadX, int frontZ, int backZ)
    {
        out.push_back(box(Part::LegFrontLeft,  legW, legH, legD, -spreadX - legW, 0, frontZ));
        out.push_back(box(Part::LegFrontRight, legW, legH, legD,  spreadX,        0, frontZ));
        out.push_back(box(Part::LegBackLeft,   legW, legH, legD, -spreadX - legW, 0, backZ));
        out.push_back(box(Part::LegBackRight,  legW, legH, legD,  spreadX,        0, backZ));
    }

    std::vector<MobType> buildTable()
    {
        std::vector<MobType> table(static_cast<size_t>(MobId::Count));

        {
            MobType& t = table[static_cast<size_t>(MobId::Sheep)];
            t.id = MobId::Sheep;
            t.name = "Sheep";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 8;
            t.width = 0.9f;
            t.height = 1.25f;
            t.walkSpeed = 1.8f;
            t.voice = Sound::MobBleat;
            t.bodyColour = rgb(228, 228, 222);
            t.headColour = rgb(222, 212, 198);
            addLegs(t.model, 4, 10, 4, 1, 4, -8);
            t.model.push_back(box(Part::Body, 10, 10, 16, -5, 10, -8));
            t.model.push_back(box(Part::Head, 6, 6, 8, -3, 12, 8));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Pig)];
            t.id = MobId::Pig;
            t.name = "Pig";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.9f;
            t.height = 0.875f;
            t.walkSpeed = 2.0f;
            t.voice = Sound::MobGrunt;
            t.voicePitch = 1.25f;
            t.bodyColour = rgb(238, 145, 145);
            t.headColour = rgb(238, 145, 145);
            addLegs(t.model, 4, 6, 4, 1, 4, -8);
            t.model.push_back(box(Part::Body, 10, 8, 16, -5, 6, -8));
            t.model.push_back(box(Part::Head, 8, 8, 8, -4, 4, 8));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Cow)];
            t.id = MobId::Cow;
            t.name = "Cow";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 10;
            t.width = 0.9f;
            t.height = 1.375f;
            t.walkSpeed = 1.8f;
            t.voice = Sound::MobGrunt;
            t.voicePitch = 0.7f;
            t.bodyColour = rgb(74, 54, 42);
            t.headColour = rgb(60, 44, 36);
            addLegs(t.model, 4, 12, 4, 1, 5, -9);
            t.model.push_back(box(Part::Body, 12, 10, 18, -6, 12, -9));
            t.model.push_back(box(Part::Head, 8, 8, 6, -4, 14, 9));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Chicken)];
            t.id = MobId::Chicken;
            t.name = "Chicken";
            t.spawnClass = SpawnClass::Passive;
            t.maxHealth = 4;
            t.width = 0.4f;
            t.height = 0.75f;
            t.walkSpeed = 1.5f;
            t.voice = Sound::MobCluck;
            t.bodyColour = rgb(234, 234, 230);
            t.headColour = rgb(234, 234, 230);
            t.eyeColour = rgb(190, 60, 40);
            t.model.push_back(box(Part::LegBackLeft,  2, 4, 2, -2, 0, -1));
            t.model.push_back(box(Part::LegBackRight, 2, 4, 2,  0, 0, -1));
            t.model.push_back(box(Part::Body, 6, 6, 8, -3, 4, -4));
            t.model.push_back(box(Part::Head, 4, 4, 4, -2, 8, 4));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Zombie)];
            t.id = MobId::Zombie;
            t.name = "Zombie";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 2.0f;
            t.walkSpeed = 1.9f;
            t.voice = Sound::MobGroan;
            t.bodyColour = rgb(58, 92, 132);
            t.headColour = rgb(88, 132, 72);
            t.eyeColour = rgb(24, 32, 24);
            // A biped: the arms stand in for the front pair of legs, so
            // they swing with the opposite leg the way an animal's do.
            t.model.push_back(box(Part::LegBackLeft,  4, 12, 4, -4, 0, -2));
            t.model.push_back(box(Part::LegBackRight, 4, 12, 4,  0, 0, -2));
            t.model.push_back(box(Part::LegFrontLeft,  4, 12, 4, -8, 12, -2));
            t.model.push_back(box(Part::LegFrontRight, 4, 12, 4,  4, 12, -2));
            t.model.push_back(box(Part::Body, 8, 12, 4, -4, 12, -2));
            t.model.push_back(box(Part::Head, 8, 8, 8, -4, 24, -4));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Skeleton)];
            t.id = MobId::Skeleton;
            t.name = "Skeleton";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 2.0f;
            t.walkSpeed = 2.1f;
            t.voice = Sound::MobRattle;
            t.bodyColour = rgb(200, 200, 196);
            t.headColour = rgb(214, 214, 210);
            t.eyeColour = rgb(20, 20, 20);
            t.model.push_back(box(Part::LegBackLeft,  2, 12, 2, -3, 0, -1));
            t.model.push_back(box(Part::LegBackRight, 2, 12, 2,  1, 0, -1));
            t.model.push_back(box(Part::LegFrontLeft,  2, 12, 2, -6, 12, -1));
            t.model.push_back(box(Part::LegFrontRight, 2, 12, 2,  4, 12, -1));
            t.model.push_back(box(Part::Body, 8, 12, 4, -4, 12, -2));
            t.model.push_back(box(Part::Head, 8, 8, 8, -4, 24, -4));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Creeper)];
            t.id = MobId::Creeper;
            t.name = "Creeper";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 20;
            t.width = 0.6f;
            t.height = 1.625f;
            t.walkSpeed = 2.0f;
            t.voice = Sound::MobHiss;
            t.bodyColour = rgb(78, 158, 62);
            t.headColour = rgb(88, 172, 70);
            t.eyeColour = rgb(18, 24, 18);
            addLegs(t.model, 4, 6, 4, 0, 2, -6);
            t.model.push_back(box(Part::Body, 8, 12, 4, -4, 6, -2));
            t.model.push_back(box(Part::Head, 8, 8, 8, -4, 18, -4));
        }
        {
            MobType& t = table[static_cast<size_t>(MobId::Spider)];
            t.id = MobId::Spider;
            t.name = "Spider";
            t.spawnClass = SpawnClass::Hostile;
            t.maxHealth = 16;
            t.width = 1.4f;
            t.height = 0.75f;
            t.walkSpeed = 2.4f;
            t.voice = Sound::MobRattle;
            t.voicePitch = 1.4f;
            t.bodyColour = rgb(44, 38, 38);
            t.headColour = rgb(56, 48, 46);
            t.eyeColour = rgb(190, 40, 40);
            addLegs(t.model, 5, 4, 2, 5, 2, -4);
            t.model.push_back(box(Part::Body, 10, 8, 14, -5, 4, -7));
            t.model.push_back(box(Part::Head, 8, 8, 8, -4, 4, 7));
        }

        return table;
    }
}

const MobType& mobType(MobId id)
{
    static const std::vector<MobType> table = buildTable();
    const size_t index = static_cast<size_t>(id);
    return table[index < table.size() ? index : 0];
}

int mobTypeCount() { return static_cast<int>(MobId::Count); }

float modelTop(const MobType& type)
{
    int top = 0;
    for (const MobBox& b : type.model) top = std::max(top, b.origin.y + b.size.y);
    return static_cast<float>(top) / 16.0f;
}
