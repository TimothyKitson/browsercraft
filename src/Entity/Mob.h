#pragma once
#include <algorithm>
#include "MobType.h"
#include "Projectiles.h"
#include "Game/Effects.h"
#include "Pathfinder.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

class World;

// One noise a mob wants made this frame. Mobs queue these rather than
// reaching for the audio engine themselves, so the physics stays testable
// without a sound card.
struct MobSound
{
    Sound id = Sound::MobPigSay;
    glm::vec3 position{ 0.0f };
    float volume = 1.0f;
    float pitch = 1.0f;
};

// A blow a mob has landed on the player this frame. Queued rather than
// applied, for the same reason the sounds are: a mob knows nothing about
// the player beyond where they are standing.
struct MobStrike
{
    int damage = 0;
    glm::vec3 from{ 0.0f };     // the mob's position, for knockback
    int blindSeconds = 0;
    int effect = -1;
    int effectAmplifier = 0;
    float effectSeconds = 0.0f;
    int attacker = -1;
};

// A mob going off. Collected like the strikes and the shots, so a mob
// never has to know what a world or an explosion is.
struct MobBlast
{
    glm::vec3 at{ 0.0f };
    float radius = 0.0f;
    int damage = 0;
    int source = -1;
    bool charged = false;
};

// A mob loosing an arrow. Collected like the strikes and the sounds,
// for the same reason: a mob knows nothing about the arrows in flight.
struct MobShot
{
    glm::vec3 from{ 0.0f };     // where it leaves the bow
    glm::vec3 velocity{ 0.0f };
    int damage = 0;
    Arrows::Kind kind = Arrows::Kind::Shaft;   // a charge rather than an arrow
    bool fangs = false;
    glm::vec3 target{ 0.0f };
    int effect = 0;
    int shooter = -1;
};

// A living thing: gravity and box collision against the voxel world, plus
// enough wits to wander, notice the player, fight them, and make noise.
// Whether the player is looking at a mob's head, which is what turns a
// neutral one hostile. Kept out of Mob so it can be checked on its own,
// with nothing but numbers.
namespace Stare
{
    // The crosshair has to land inside this much of the head's centre.
    // An enderman's head is half a block wide; a little under that is
    // forgiving enough to aim at and still tight enough that walking
    // past one with it in the corner of your eye leaves it alone.
    constexpr float HEAD_RADIUS = 0.32f;
    constexpr float RANGE = 64.0f;

    // True when the look ray passes through the head. The test is where
    // the ray goes, not how near the mob is to the middle of the screen:
    // an angular test would provoke at sixty blocks what it ignored at
    // two, since the same angle covers a wider circle further out.
    bool atHead(const glm::vec3& eye, const glm::vec3& look,
                const glm::vec3& feet, float height,
                float range = RANGE, float headRadius = HEAD_RADIUS);
}

class Mob
{
public:
    Mob(MobId type, glm::vec3 feetPosition, uint32_t seed, bool baby = false);

    void update(float deltaTime, const World& world, const glm::vec3& playerPosition,
                float daylight = 0.0f);

    // Set while the sun is burning it: the renderer tints it, and it is
    // losing health once a second until it finds shade or dies.
    bool burning() const { return m_burnTimer > 0.0f; }
    void ignite(float seconds) { if (!type().fireproof) m_burnTimer = std::max(m_burnTimer, seconds); }

    // Hurt an animal and it bolts rather than ambles: twice its walking
    // pace, for long enough to actually get away from you.
    static constexpr float PANIC_SECONDS = 5.0f;
    static constexpr float PANIC_SPEED = 2.0f;

    bool panicking() const { return m_panicTimer > 0.0f; }
    float currentSpeed() const;

    static constexpr float SUNLIGHT_BURN_INTERVAL = 1.0f;
    static constexpr float HURT_VOICE_SECONDS = 0.55f;
    static constexpr float SUNLIGHT_DAYLIGHT = 0.5f;

    // Whether the sun is on it right now. Pulled out of the update so
    // --selftest can ask the question without a world to stand in: a
    // block of wool overhead or a metre of water is all it takes.
    static bool burnsNow(const MobType& type, float daylight, int skyLight, bool inLiquid);

    void damage(int amount, const glm::vec3& fromDirection);
    bool strike(int amount, const glm::vec3& fromDirection);
    bool alive() const { return m_health > 0; }
    bool finished() const { return m_removeTimer <= 0.0f && !alive(); }

    MobId typeId() const { return m_type; }

    // The seed it was born with, untouched. m_rng below is the same
    // number to begin with but is stirred every time anything is asked
    // of it, so it is no use for deciding something that has to stay
    // the same for the life of the mob -- a villager's profession, for
    // one, which would otherwise change as it wandered about.
    uint32_t birthSeed() const { return m_birthSeed; }
    uint32_t serial() const { return m_serial; }
    enum class Affliction { Glowing, Poison, Slowness };
    void afflict(Affliction kind, float seconds)
    {
        float& timer = kind == Affliction::Glowing ? m_glowTimer : kind == Affliction::Poison ? m_poisonTimer : m_slowTimer;
        timer = std::max(timer, seconds);
    }
    bool glowing() const { return m_glowTimer > 0.0f || m_effects.has(Effects::Kind::Glowing); }
    void giveEffect(Effects::Kind kind, int amplifier, float seconds);
    static void setNoticeScale(float scale);
    void pierceArmour(int breach) { m_breach = breach; }
    static int naturalArmour(MobId id)
    {
        switch (id)
        {
            case MobId::Zombie: case MobId::Husk: case MobId::Drowned: case MobId::ZombieVillager:
            case MobId::ZombiePigman: return 2;
            case MobId::Wither: return 4;
            default: return 0;
        }
    }
    void markPlayerHit(int how, float distance)
    {
        m_playerHitAge = 0.0f;
        m_playerHitHow = how;
        m_playerHitDistance = distance;
    }
    bool hitByPlayer() const { return m_playerHitAge < 5.0f; }
    int playerHitHow() const { return m_playerHitHow; }
    float playerHitDistance() const { return m_playerHitDistance; }
    static float noticeScale();
    const Effects::Set& effects() const { return m_effects; }
    bool invisible() const { return m_effects.has(Effects::Kind::Invisibility); }
    int takeSilverfish()
    {
        const int count = m_silverfish;
        m_silverfish = 0;
        return count;
    }
    bool slowed() const { return m_slowTimer > 0.0f; }
    bool poisoned() const { return m_poisonTimer > 0.0f; }
    bool screamingGoat() const { return (m_birthSeed % 50u) == 7u; }
    int hornsLeft() const { return m_horns; }
    void loseHorn() { if (m_horns > 0) --m_horns; }
    bool ramReady(float deltaTime)
    {
        m_ramTimer -= deltaTime;
        if (m_ramTimer > 0.0f) return false;
        m_ramTimer = 30.0f + static_cast<float>((m_birthSeed * 2654435761u + m_serial * 40503u) % 271u);
        return true;
    }
    const std::string& customName() const { return m_customName; }
    void setCustomName(const std::string& name) { m_customName = name; }
    bool leashed() const { return m_leashed; }
    void keepFromDespawning() { m_persistent = true; }
    void vanish() { m_health = 0; m_removeTimer = 0.0f; m_deathReported = true; }
    float growthBoost() const { return m_growthBoost; }
    static constexpr float TADPOLE_SECONDS = 1200.0f;
    bool persistent() const { return m_persistent; }
    void setLeashed(bool on) { m_leashed = on; }
    static bool leashable(const MobType& type)
    {
        return !type.inanimate && !type.stationary && type.spawnClass != SpawnClass::Hostile;
    }
    void setBirthSeed(uint32_t seed) { m_birthSeed = seed; }

    // A neutral mob that has been given a reason. Once set it stays
    // set: Minecraft's enderman does not forgive, and a mob that
    // cooled off mid-fight would be worse than one that never warmed.
    bool provoked() const { return m_provoked; }
    void provoke() { m_provoked = true; }

    // Whether it would attack the player on sight as things stand --
    // true for anything hostile, and for a neutral one once provoked.
    bool hostileNow() const;
    // The same rule on its own: a neutral mob waits to be provoked,
    // anything else does not wait at all.
    static bool hostileNow(const MobType& type, bool provoked);
    const MobType& type() const { return mobType(m_type); }

    glm::vec3 position() const { return m_position; }
    glm::vec3 velocity() const { return m_velocity; }
    // Degrees, the same convention as the camera and the player model.
    float yaw() const { return m_yaw; }
    int health() const { return m_health; }
    float spawnPhase() const { return m_spawnPhase; }
    float laserCharge() const { return m_laserCharge; }
    static constexpr float LASER_SECONDS = 2.0f;
    float lastRange() const { return m_lastRange; }
    bool aboutToShoot() const
    {
        return type().arrowDamage > 0 && m_shootTimer > 0.0f && m_shootTimer < 0.5f &&
               m_lastRange < type().arrowRange;
    }

    // Put health back, never past what the species starts with. The
    // dragon's crystals are the only thing that uses this: everything
    // else in the world only ever loses health.
    void heal(int amount);

    // Non-zero while flashing from a hit.
    float hurtFlash() const { return m_hurtFlash; }
    float darkTime() const { return m_darkTime; }
    float glowLight() const;
    bool illusioned() const { return m_illusionTime > 0.0f; }
    float illusionTime() const { return m_illusionTime; }
    glm::vec3 mirrorOffset(int copy) const;
    void shuffleMirrors();
    bool blindedPlayer() const { return m_blindedPlayer; }
    float zombifyTime() const { return m_zombifyTime; }
    void addZombifyTime(float seconds) { m_zombifyTime = seconds > 0.0f ? m_zombifyTime + seconds : 0.0f; }
    // 0 standing, 1 at walking pace; drives how far the legs swing.
    float gaitAmount() const { return m_gaitAmount; }
    // Radians around the walk cycle.
    float gait() const { return m_gait; }
    // 1 while alive, falling to 0 as a dead one keels over.
    float deathFade() const;

    // How far the shulker's lid is open, 0 shut and 1 wide. Nothing
    // else in the game has one, so it rests at 0 everywhere else.
    float peek() const { return m_peek; }

    // How long it has been alive, for the one animation that is a
    // clock rather than a stride: an open lid bobs where it stands.
    float age() const { return m_age; }

    std::vector<MobSound>& sounds() { return m_sounds; }
    std::vector<MobStrike>& strikes() { return m_strikes; }
    std::vector<MobShot>& shots() { return m_shots; }
    std::vector<MobBlast>& blasts() { return m_blasts; }

    // 0 while the fuse is unlit, running to 1 as it burns down. The
    // renderer swells the mob by it, which is the warning you get.
    float fuse() const;

    // True exactly once, on the first call after it dies. A corpse
    // lingers for a moment before it is dropped, so whatever it leaves
    // behind cannot wait for the removal.
    bool takeDeathReport();

    // A chicken lays an egg every few minutes, which is Minecraft's
    // and is the only way an egg gets into the game. True exactly
    // once per egg, on the first call after one is due, for the same
    // reason the death report is: whatever it leaves belongs where it
    // was standing when it laid it.
    static constexpr float EGG_SECONDS = 180.0f;
    static constexpr float WITHER_SPAWN_SECONDS = 11.0f;
    static constexpr float WITHER_SPAWN_BLAST = 7.0f;
    bool takeEggReport();
    bool takeGrowthReport();

    // --- breeding ---
    //
    // A fed animal is "in love" for half a minute. Two of the same kind
    // in love and standing close enough pair off, and one of them
    // reports the birth.
    static constexpr float LOVE_SECONDS = 30.0f;
    static constexpr float BREEDING_COOLDOWN = 60.0f;
    static constexpr float BABY_SECONDS = 300.0f;   // five minutes to grow up
    static constexpr float BABY_SCALE = 0.5f;

    bool baby() const { return m_babyTimer > 0.0f; }
    bool inLove() const { return m_loveTimer > 0.0f; }
    bool canBreed() const { return !baby() && m_breedTimer <= 0.0f && alive(); }
    // Half size while young, which is also what shrinks its hitbox.
    // Shoved by something that is not a blow: a wind charge's burst.
    void shove(const glm::vec3& push) { m_velocity += push; m_onGround = false; }

    // Stands where it is for this long, as a villager does while it is
    // being traded with.
    void holdStill(float seconds) { m_holdTimer = std::max(m_holdTimer, seconds); }
    bool held() const { return m_holdTimer > 0.0f; }

    float scale() const { return (baby() ? BABY_SCALE : 1.0f) * type().modelScale; }
    float width() const { return type().width * scale(); }
    float height() const { return type().height * scale(); }

    // --- pathfinding ---
    //
    // A mob does not search for itself. It says it would like a route,
    // the manager works out how many searches the frame can afford, and
    // the answer comes back here. That is what keeps forty-eight of them
    // from each searching the world every frame.
    static constexpr float REPATH_INTERVAL = 1.0f;

    bool wantsPath() const;
    glm::ivec3 pathGoal() const { return m_pathGoal; }
    void setPath(std::vector<glm::ivec3> path, const glm::ivec3& goal);
    bool hasPath() const { return m_pathIndex < m_path.size(); }
    int pathLength() const { return static_cast<int>(m_path.size()); }

    // Accepts the food and falls in love. False if it is not interested.
    bool feed(StackId food);

    // Tamed, and what it takes. A wolf given a bone follows whoever
    // gave it; nothing else in the game is tameable yet, so the item
    // is asked for by type rather than kept on one.
    static StackId tamingFood(MobId type);
    bool tamed() const { return m_tamed; }

    // A tamed animal told to stay. It keeps its place until it is told
    // otherwise, which is what makes a wolf a guard rather than a
    // shadow that follows you into every cave.
    bool sitting() const { return m_sitting; }
    void toggleSit() { if (m_tamed) m_sitting = !m_sitting; }
    void restore(int health, bool tamed, bool sitting) { m_health = health; m_tamed = tamed; m_sitting = tamed && sitting; }
    // Only a tamed animal of a kind that sits at all.
    static bool canSit(MobId type, bool tamed);
    // Returns true when this actually tamed it, which is the caller's
    // cue to spend the bone.
    bool tameWith(StackId food);

    // How near a tamed one tries to stay to whoever it follows, and how
    // far it will let them get before coming after them.
    static constexpr float HEEL_RANGE = 10.0f;

    // How hard a ridden mob jumps when the rider asks.
    static constexpr float RIDDEN_JUMP = 8.0f;

    // How a hull sits in the water: how deep it rides, how hard the
    // surface pulls it back, how fast that pull is bled off, and how far
    // up it will look for the top of the water it is in.
    static constexpr float FLOAT_DRAFT = 0.02f;
    static constexpr float FLOAT_PULL = 24.0f;
    static constexpr float FLOAT_DAMPING = 7.0f;
    static constexpr int FLOAT_RISE = 8;

    // How hard the air holds a hovering mob at its own height, and how
    // fast that pull is bled off. The same spring as the waterline,
    // because it is the same problem: a height to settle at.
    static constexpr float HOVER_PULL = 9.0f;
    static constexpr float HOVER_DAMPING = 4.0f;
    static constexpr int HOVER_DROP = 24;

    // One step of the vertical rule, whichever applies: the spring
    // towards the waterline for something afloat, the fall for
    // everything else. One function and not two, because the bug worth
    // keeping out is applying both at once.
    static float verticalStep(float velocityY, float positionY, float target,
                              bool held, float pull, float damping, float deltaTime);

    // The two the water uses, so the old call reads the same.
    static float verticalStep(float velocityY, float positionY, float waterline,
                              bool afloat, float deltaTime)
    {
        return verticalStep(velocityY, positionY, waterline, afloat,
                            FLOAT_PULL, FLOAT_DAMPING, deltaTime);
    }

    // How high a rail's own surface stands in its cell, which is where
    // a cart's wheels sit. The mesher draws the panel at the same
    // height, so the two agree by sharing the number.
    static constexpr float RAIL_LIFT = 0.0625f;

    // Where the track puts a cart and which way it sends it. The cart
    // leaves by whichever of the rail's two ends it is already pointed
    // at, and is held on the middle of the track across its travel --
    // which is what keeps it on the rails rather than merely near them.
    // False when the block is not a rail, or is a rail with fewer than
    // two ends, which cannot happen but would be a hole if it did.
    static bool railGuide(BlockId rail, const glm::ivec3& cell, const glm::vec3& position,
                          float yawDegrees, glm::vec3& snapped, float& exitYaw);

    // Which way each of the four compass points faces, and the heading
    // that travels along it.
    // A cart keeps what it has been given. Pushed, it gains speed up
    // to its own limit; left alone it coasts and slowly gives it back.
    // Held at zero rather than allowed to go negative, or a stopped
    // cart would set off backwards.
    static constexpr float RAIL_PUSH = 7.0f;
    static constexpr float RAIL_DRAG = 1.1f;
    static float railSpeedStep(float speed, bool pushing, float maxSpeed, float deltaTime);

    static glm::vec3 railDirection(int facing);
    static float railYaw(int facing);

    // --- being ridden ---
    //
    // A ridden mob stops deciding where to go and does what the rider
    // asks. Steering is handed in rather than read, because the mob
    // has no idea a player exists and should not learn.
    // Who may be got on. A horse has to be tamed and grown first; a
    // boat belongs to whoever finds it, and nothing else is transport.
    static bool canBeRidden(const MobType& type, bool tamed, bool baby, bool saddled = false)
    {
        if (!type.rideable || baby) return false;
        if (type.saddleRide) return saddled;
        return type.inanimate || tamed;
    }
    void boostRide(float seconds) { m_rideBoost = seconds; m_rideBoostLength = seconds; }
    bool rideBoosted() const { return m_rideBoost > 0.0f; }

    bool ridden() const { return m_ridden; }
    bool saddled() const { return m_saddled; }
    void setSaddled(bool on) { m_saddled = on; }
    uint16_t horseArmour() const { return m_horseArmour; }
    void setHorseArmour(uint16_t armour) { m_horseArmour = armour; }
    int shulkerColour() const { return m_shulkerColour; }
    void setShulkerColour(int colour) { m_shulkerColour = (colour >= 0 && colour < 16) ? colour : 16; }
    int variant() const { return m_variant; }
    void setYaw(float yaw) { m_yaw = yaw; m_goalYaw = yaw; }
    void setSelfPush(bool pushing) { m_selfPush = pushing; }
    void markShotByMob() { m_shotByMob = true; }
    bool charged() const { return m_charged; }
    int woolColour() const { return m_variant & 15; }
    bool sheared() const { return (m_variant & 16) != 0; }
    void setWool(int colour, bool shorn) { m_variant = (colour & 15) | (shorn ? 16 : 0); }
    bool takeGrazeReport() { const bool was = m_grazed; m_grazed = false; return was; }
    bool curing() const { return m_cureTimer > 0.0f; }
    void requestTeleport() { m_wantsTeleport = true; }
    bool teleportRandom(const World& world, float range);
    void startCure(float seconds) { m_cureTimer = seconds; }
    bool cureDone(float deltaTime) { if (m_cureTimer <= 0.0f) return false; m_cureTimer -= deltaTime; return m_cureTimer <= 0.0f; }
    void setCharged(bool on) { m_charged = on; }
    void markChargedBlast() { m_chargedBlast = true; }
    bool chargedBlastHit() const { return m_chargedBlast; }
    bool rolled() const { return m_rolled; }
    void scare(float seconds)
    {
        if (!m_rolled) m_rolledJustNow = true;
        m_rolled = true;
        m_rollHold = std::max(m_rollHold, seconds);
    }
    bool takeRollReport() { const bool was = m_rolledJustNow; m_rolledJustNow = false; return was; }
    int armourWear() const { return m_armourWear; }
    void setArmourWear(int wear) { m_armourWear = wear; }
    int armourDye() const { return m_armourDye; }
    void setArmourDye(int dye) { m_armourDye = dye; }
    bool takeArmourBreak() { const bool was = m_armourBroke; m_armourBroke = false; return was; }
    bool shotByMob() const { return m_shotByMob; }
    float railSpeed() const { return m_railSpeed; }
    void setVariant(int variant) { m_variant = variant; }
    void setRidden(bool on);
    void steer(float yawDegrees, bool forward, bool jump, float jumpPower = 1.0f);
    bool onGround() const { return m_onGround; }

    // Where a rider sits, which is not where the mob stands.
    glm::vec3 saddle() const
    {
        return m_position + glm::vec3(0.0f, type().saddleHeight, 0.0f);
    }

    // A tamed one set on something. The heading is handed in rather
    // than worked out here, because the mob cannot see the other mobs
    // -- only whoever keeps them all can.
    void huntTowards(const glm::vec3& at);
    bool hunting() const { return m_huntTimer > 0.0f; }

    // How hard a wolf bites and how often. Returns true on the frame a
    // bite actually lands, which is the caller's cue to apply it.
    static constexpr int BITE_DAMAGE = 4;
    static constexpr float BITE_INTERVAL = 1.0f;
    static constexpr float HUNT_RANGE = 14.0f;
    static constexpr float BITE_REACH = 1.8f;
    bool tryBite(float deltaTime);

    // A villager's trade. Set from the building it was born or spawned
    // in, so the smith lives in the smithy. -1 means nobody told it,
    // and the caller falls back to drawing one from the birth seed --
    // which is what a villager met out in the country still does.
    int professionSlot() const { return m_profession; }
    void setProfessionSlot(int trade) { m_profession = trade; }

    // Villagers turn in at night. Asleep they stand still, which is
    // most of what makes a village read as asleep rather than as a
    // field of people milling about in the dark.
    bool asleep() const { return m_asleep; }

    // Whether this kind of mob would be asleep in this light. Pure, so
    // the rule can be checked without a world to walk about in.
    // Anything just startled is awake whatever the hour.
    static constexpr float SLEEP_BELOW = 0.25f;
    static bool sleepsAt(MobId type, float daylight, bool startled)
    {
        return type == MobId::Villager && daylight < SLEEP_BELOW && !startled;
    }

    // Puts one in love without feeding it, which is how villagers pair
    // off: nobody hands a villager a carrot.
    void makeWilling();

    // Where it belongs, and how far it will stray from there. A
    // villager given one turns back when it reaches the edge, so a
    // village is still a village an hour later rather than a set of
    // empty houses with everyone over the next hill.
    void setHome(const glm::vec3& where, float radius);
    bool hasHome() const { return m_homeRadius > 0.0f; }
    glm::vec3 home() const { return m_home; }

    // Whether something standing here would be turning back. Pure, so
    // the rule can be checked without a world.
    static bool wouldTurnBack(const glm::vec3& at, const glm::vec3& home, float radius);
    // Called on both parents once they have paired off.
    void onBred();

private:
    float random01();
    void emit(Sound id, float volume, float pitch);
    bool collidesAt(const World& world, const glm::vec3& feet) const;
    void moveAxis(const World& world, float delta, int axis);
    void chooseNewGoal(const glm::vec3& playerPosition);

    MobId m_type;
    glm::vec3 m_position{ 0.0f };
    glm::vec3 m_velocity{ 0.0f };
    float m_yaw = 0.0f;
    int m_shulkerColour = 16;
    int m_variant = 0;
    bool m_selfPush = false;
    bool m_shotByMob = false;
    bool m_charged = false;
    bool m_grazed = false;
    float m_cureTimer = 0.0f;
    bool m_wantsTeleport = false;
    float m_grazeTimer = 20.0f;
    bool m_chargedBlast = false;
    bool m_rolled = false;
    bool m_rolledJustNow = false;
    float m_rollHold = 0.0f;
    int m_armourWear = 0;
    int m_armourDye = -1;
    bool m_armourBroke = false;
    int m_health = 1;
    bool m_onGround = false;

    float m_goalYaw = 0.0f;
    float m_goalTimer = 0.0f;
    bool m_moving = false;

    std::vector<glm::ivec3> m_path;
    size_t m_pathIndex = 0;
    glm::ivec3 m_pathGoal{ 0 };
    float m_repathTimer = 0.0f;
    bool m_chasing = false;

    // Steers towards the next waypoint. True while there is one.
    void burnInSunlight(float deltaTime, const World& world, float daylight);
    bool followPath();
    // A heading that is actually clear, for when the chosen one is not.

    bool floatTowardsSurface(const World& world, float deltaTime);
    bool hoverAboveGround(const World& world, float deltaTime);
    void steerAroundObstacle(const World& world);

    float m_attackTimer = 0.0f;
    float m_strikeGuard = 0.0f;
    int m_lastStrike = 0;
    float m_ambientTimer = 0.0f;
    float m_hurtVoiceTimer = 0.0f;
    float m_shootTimer = 0.0f;
    float m_spawnPhase = 0.0f;
    float m_laserCharge = 0.0f;
    float m_lastRange = 1.0e9f;
    float m_fuseTimer = 0.0f;
    bool m_lit = false;
    float m_stepDistance = 0.0f;
    float m_gait = 0.0f;
    float m_gaitAmount = 0.0f;
    glm::vec3 m_home{ 0.0f };
    float m_homeRadius = 0.0f;
    int m_profession = -1;
    bool m_asleep = false;
    bool m_tamed = false;
    bool m_sitting = false;
    float m_holdTimer = 0.0f;
    bool m_ridden = false;
    uint32_t m_serial = 0;
    float m_glowTimer = 0.0f;
    Effects::Set m_effects;
    int m_breach = 0;
    int m_silverfish = 0;
    float m_playerHitAge = 1.0e9f;
    int m_playerHitHow = 0;
    float m_playerHitDistance = 0.0f;
    float m_poisonTimer = 0.0f;
    float m_poisonClock = 0.0f;
    float m_slowTimer = 0.0f;
    int m_horns = 2;
    float m_ramTimer = 20.0f;
    std::string m_customName;
    bool m_leashed = false;
    float m_rideBoost = 0.0f;
    float m_rideBoostLength = 0.0f;
    bool m_steerForward = false;
    bool m_steerJump = false;
    float m_jumpPower = 1.0f;
    bool m_saddled = false;
    uint16_t m_horseArmour = 0;
    float m_railSpeed = 0.0f;
    float m_huntTimer = 0.0f;
    float m_biteTimer = 0.0f;
    float m_loveTimer = 0.0f;
    float m_breedTimer = 0.0f;
    float m_babyTimer = 0.0f;
    float m_hurtFlash = 0.0f;
    float m_darkTime = 0.0f;
    float m_illusionTime = 0.0f;
    float m_illusionCooldown = 0.0f;
    float m_blindDelay = 9.0f;
    bool m_blindedPlayer = false;
    float m_mirrorBlend = 0.0f;
    float m_mirrorCollapse = 0.0f;
    glm::vec3 m_mirrors[2][4] = {};
    float m_zombifyTime = 0.0f;
    float m_fallTopY = 0.0f;
    float m_air = 15.0f;
    float m_hazardTimer = 0.0f;
    bool m_fallTracked = false;
    float m_panicTimer = 0.0f;
    float m_burnTimer = 0.0f;
    float m_burnTick = 0.0f;
    float m_removeTimer = 0.6f;
    bool m_deathReported = false;
    float m_eggTimer = EGG_SECONDS;
    bool m_grewUp = false;
    bool m_eggReady = false;
    float m_peek = 0.0f;
    float m_age = 0.0f;

    uint32_t m_rng = 1;
    uint32_t m_birthSeed = 1;
    bool m_persistent = false;
    float m_growthBoost = 0.0f;
    bool m_provoked = false;
    std::vector<MobSound> m_sounds;
    std::vector<MobStrike> m_strikes;
    std::vector<MobShot> m_shots;
    std::vector<MobBlast> m_blasts;
};
