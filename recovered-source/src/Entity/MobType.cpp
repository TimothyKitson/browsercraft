#include "MobType.h"
#include "Renderer/AtlasTiles.h"

// Box models are in block units measured from the mob's feet, so a sheep's
// body sits at y = 0.75 because that is where it rides above the ground.
//
// The sound paths follow the layout the engine already uses for blocks and
// footsteps: a name here plus a variant number plus ".ogg", resolved under
// assets/sounds/. A variant count of zero means the engine never asks for that
// event, and a count higher than the number of files present simply means the
// missing ones are skipped, so a partial sound pack still works.

namespace
{
    std::vector<ModelBox> quadrupedModel(float bodyY, float bodyLen, float bodyH,
                                         float headFwd, float headSize,
                                         float legH, int bodyTile, int headTile)
    {
        const float legOffX = 0.22f;
        const float legOffZ = bodyLen * 0.5f - 0.18f;
        const glm::vec3 legHalf(0.09f, legH * 0.5f, 0.09f);

        return {
            { { 0.0f, bodyY, 0.0f }, { 0.28f, bodyH * 0.5f, bodyLen * 0.5f }, bodyTile },
            { { 0.0f, bodyY + 0.08f, headFwd }, glm::vec3(headSize * 0.5f), headTile },
            { { -legOffX, legH * 0.5f,  legOffZ }, legHalf, bodyTile },
            { {  legOffX, legH * 0.5f,  legOffZ }, legHalf, bodyTile },
            { { -legOffX, legH * 0.5f, -legOffZ }, legHalf, bodyTile },
            { {  legOffX, legH * 0.5f, -legOffZ }, legHalf, bodyTile },
        };
    }

    std::vector<ModelBox> bipedModel(float bodyTile, float headTile)
    {
        const int body = static_cast<int>(bodyTile);
        const int head = static_cast<int>(headTile);
        return {
            { { 0.0f, 1.12f, 0.0f }, { 0.25f, 0.37f, 0.14f }, body },  // torso
            { { 0.0f, 1.72f, 0.0f }, glm::vec3(0.25f), head },         // head
            { { -0.36f, 1.12f, 0.0f }, { 0.11f, 0.37f, 0.11f }, body },// arms
            { {  0.36f, 1.12f, 0.0f }, { 0.11f, 0.37f, 0.11f }, body },
            { { -0.13f, 0.37f, 0.0f }, { 0.11f, 0.37f, 0.11f }, body },// legs
            { {  0.13f, 0.37f, 0.0f }, { 0.11f, 0.37f, 0.11f }, body },
        };
    }

    std::vector<MobType> buildTable()
    {
        std::vector<MobType> table(static_cast<size_t>(MobId::Count));

        MobType& sheep = table[static_cast<size_t>(MobId::Sheep)];
        sheep.id = MobId::Sheep;
        sheep.name = "Sheep";
        sheep.spawnClass = SpawnClass::Passive;
        sheep.maxHealth = 8;
        sheep.width = 0.9f;
        sheep.height = 1.3f;
        sheep.walkSpeed = 2.0f;
        sheep.soundAmbient = "mob/sheep/say";
        sheep.soundHurt = "mob/sheep/say";
        sheep.soundDeath = "mob/sheep/say";
        sheep.soundStep = "mob/sheep/step";
        sheep.ambientVariants = 3;
        sheep.hurtVariants = 3;
        sheep.deathVariants = 3;
        sheep.stepVariants = 5;
        sheep.model = quadrupedModel(0.78f, 0.95f, 0.52f, 0.52f, 0.42f, 0.52f, Tiles::Wool, Tiles::Wool);

        MobType& pig = table[static_cast<size_t>(MobId::Pig)];
        pig.id = MobId::Pig;
        pig.name = "Pig";
        pig.spawnClass = SpawnClass::Passive;
        pig.maxHealth = 10;
        pig.width = 0.9f;
        pig.height = 0.9f;
        pig.walkSpeed = 2.1f;
        pig.soundAmbient = "mob/pig/say";
        pig.soundHurt = "mob/pig/say";
        pig.soundDeath = "mob/pig/death";
        pig.soundStep = "mob/pig/step";
        pig.ambientVariants = 3;
        pig.hurtVariants = 3;
        pig.deathVariants = 1;
        pig.stepVariants = 5;
        pig.model = quadrupedModel(0.62f, 0.9f, 0.48f, 0.5f, 0.38f, 0.38f, Tiles::FlowerRed, Tiles::FlowerRed);

        MobType& cow = table[static_cast<size_t>(MobId::Cow)];
        cow.id = MobId::Cow;
        cow.name = "Cow";
        cow.spawnClass = SpawnClass::Passive;
        cow.maxHealth = 10;
        cow.width = 0.9f;
        cow.height = 1.4f;
        cow.walkSpeed = 2.0f;
        cow.soundAmbient = "mob/cow/say";
        cow.soundHurt = "mob/cow/hurt";
        cow.soundDeath = "mob/cow/hurt";
        cow.soundStep = "mob/cow/step";
        cow.ambientVariants = 4;
        cow.hurtVariants = 3;
        cow.deathVariants = 3;
        cow.stepVariants = 4;
        cow.model = quadrupedModel(0.86f, 1.0f, 0.56f, 0.56f, 0.44f, 0.58f, Tiles::Dirt, Tiles::Dirt);

        MobType& chicken = table[static_cast<size_t>(MobId::Chicken)];
        chicken.id = MobId::Chicken;
        chicken.name = "Chicken";
        chicken.spawnClass = SpawnClass::Passive;
        chicken.maxHealth = 4;
        chicken.width = 0.4f;
        chicken.height = 0.7f;
        chicken.walkSpeed = 1.6f;
        chicken.soundAmbient = "mob/chicken/say";
        chicken.soundHurt = "mob/chicken/hurt";
        chicken.soundDeath = "mob/chicken/hurt";
        chicken.soundStep = "mob/chicken/step";
        chicken.ambientVariants = 3;
        chicken.hurtVariants = 3;
        chicken.deathVariants = 3;
        chicken.stepVariants = 2;
        chicken.model = quadrupedModel(0.42f, 0.4f, 0.3f, 0.26f, 0.26f, 0.26f, Tiles::SnowTile, Tiles::FlowerYellow);

        MobType& zombie = table[static_cast<size_t>(MobId::Zombie)];
        zombie.id = MobId::Zombie;
        zombie.name = "Zombie";
        zombie.spawnClass = SpawnClass::Hostile;
        zombie.maxHealth = 20;
        zombie.width = 0.6f;
        zombie.height = 1.95f;
        zombie.walkSpeed = 1.6f;
        zombie.soundAmbient = "mob/zombie/say";
        zombie.soundHurt = "mob/zombie/hurt";
        zombie.soundDeath = "mob/zombie/death";
        zombie.soundStep = "mob/zombie/step";
        zombie.ambientVariants = 3;
        zombie.hurtVariants = 2;
        zombie.deathVariants = 1;
        zombie.stepVariants = 5;
        zombie.model = bipedModel(Tiles::Leaves, Tiles::Leaves);

        MobType& skeleton = table[static_cast<size_t>(MobId::Skeleton)];
        skeleton.id = MobId::Skeleton;
        skeleton.name = "Skeleton";
        skeleton.spawnClass = SpawnClass::Hostile;
        skeleton.maxHealth = 20;
        skeleton.width = 0.6f;
        skeleton.height = 1.95f;
        skeleton.walkSpeed = 1.8f;
        skeleton.soundAmbient = "mob/skeleton/say";
        skeleton.soundHurt = "mob/skeleton/hurt";
        skeleton.soundDeath = "mob/skeleton/death";
        skeleton.soundStep = "mob/skeleton/step";
        skeleton.ambientVariants = 3;
        skeleton.hurtVariants = 4;
        skeleton.deathVariants = 1;
        skeleton.stepVariants = 4;
        skeleton.model = bipedModel(Tiles::SnowTile, Tiles::SnowTile);

        MobType& creeper = table[static_cast<size_t>(MobId::Creeper)];
        creeper.id = MobId::Creeper;
        creeper.name = "Creeper";
        creeper.spawnClass = SpawnClass::Hostile;
        creeper.maxHealth = 20;
        creeper.width = 0.6f;
        creeper.height = 1.7f;
        creeper.walkSpeed = 1.5f;
        creeper.soundAmbient = "";
        creeper.soundHurt = "mob/creeper/say";
        creeper.soundDeath = "mob/creeper/death";
        creeper.soundStep = "";
        creeper.ambientVariants = 0;
        creeper.hurtVariants = 4;
        creeper.deathVariants = 1;
        creeper.stepVariants = 0;
        creeper.model = {
            { { 0.0f, 0.85f, 0.0f }, { 0.22f, 0.52f, 0.14f }, Tiles::TallGrass },
            { { 0.0f, 1.52f, 0.0f }, glm::vec3(0.22f), Tiles::TallGrass },
            { { -0.14f, 0.16f,  0.18f }, { 0.11f, 0.16f, 0.11f }, Tiles::TallGrass },
            { {  0.14f, 0.16f,  0.18f }, { 0.11f, 0.16f, 0.11f }, Tiles::TallGrass },
            { { -0.14f, 0.16f, -0.18f }, { 0.11f, 0.16f, 0.11f }, Tiles::TallGrass },
            { {  0.14f, 0.16f, -0.18f }, { 0.11f, 0.16f, 0.11f }, Tiles::TallGrass },
        };

        MobType& spider = table[static_cast<size_t>(MobId::Spider)];
        spider.id = MobId::Spider;
        spider.name = "Spider";
        spider.spawnClass = SpawnClass::Hostile;
        spider.maxHealth = 16;
        spider.width = 1.4f;
        spider.height = 0.9f;
        spider.walkSpeed = 2.6f;
        spider.soundAmbient = "mob/spider/say";
        spider.soundHurt = "mob/spider/say";
        spider.soundDeath = "mob/spider/death";
        spider.soundStep = "mob/spider/step";
        spider.ambientVariants = 4;
        spider.hurtVariants = 4;
        spider.deathVariants = 1;
        spider.stepVariants = 4;
        spider.model = {
            { { 0.0f, 0.45f, 0.0f }, { 0.35f, 0.2f, 0.45f }, Tiles::Obsidian },
            { { 0.0f, 0.5f, 0.52f }, glm::vec3(0.22f), Tiles::Obsidian },
            { { -0.5f, 0.3f,  0.2f }, { 0.22f, 0.07f, 0.07f }, Tiles::Obsidian },
            { {  0.5f, 0.3f,  0.2f }, { 0.22f, 0.07f, 0.07f }, Tiles::Obsidian },
            { { -0.5f, 0.3f, -0.2f }, { 0.22f, 0.07f, 0.07f }, Tiles::Obsidian },
            { {  0.5f, 0.3f, -0.2f }, { 0.22f, 0.07f, 0.07f }, Tiles::Obsidian },
        };

        return table;
    }
}

const MobType& mobType(MobId id)
{
    static const std::vector<MobType> table = buildTable();
    const size_t index = static_cast<size_t>(id);
    return table[index < table.size() ? index : 0];
}

int mobTypeCount()
{
    return static_cast<int>(MobId::Count);
}
