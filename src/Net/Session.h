#pragma once
#include "Protocol.h"
#include "Transport.h"
#include <memory>
#include <string>
#include <set>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

class World;

namespace Net
{
    enum class Role
    {
        Offline,
        Host,   // this machine owns the world
        Guest   // someone else does
    };

    enum class Status
    {
        Idle,
        Connecting,   // socket still handshaking
        Loading,      // connected; waiting for the world handshake and deltas
        Playing,
        Failed
    };

    // Somebody else, drawn in the world. Positions arrive a few times a
    // second, so each one is held alongside the previous and blended --
    // otherwise everyone teleports in 20 Hz steps.
    struct RemotePlayer
    {
        uint32_t id = 0;
        std::string name;
        glm::vec3 position{ 0.0f };
        glm::vec3 previous{ 0.0f };
        float yaw = 0.0f;
        float pitch = 0.0f;
        float blend = 1.0f;
        float silence = 0.0f;   // seconds since the last update
        bool sneaking = false;
        Appearance look;
        uint16_t mainHand = 0;
        uint16_t offHand = 0;
        uint8_t swingCount = 0;
        bool swingOff = false;
        bool leftHanded = false;
        float swingTimer = 0.0f;

        // False until they have told us where they are. Someone who has
        // joined but not yet moved would otherwise be drawn standing at
        // the world origin, which reads as a bug rather than as a player.
        bool positioned = false;

        glm::vec3 smoothed() const { return glm::mix(previous, position, glm::clamp(blend, 0.0f, 1.0f)); }
    };

    struct MobSnap
    {
        uint32_t serial = 0;
        uint16_t type = 0;
        glm::vec3 position{ 0.0f };
        float yaw = 0.0f;
        int health = 1;
        int variant = 0;
        uint32_t seed = 1;
        int size = 0;
        int profession = -1;
        int shulkerColour = 16;
        int armourDye = -1;
        uint16_t flags = 0;
        uint16_t horseArmour = 0;
        uint16_t gear[6] = { 0, 0, 0, 0, 0, 0 };
        std::string name;
    };

    enum MobFlags : uint16_t
    {
        MobBaby = 1 << 0,
        MobTamed = 1 << 1,
        MobSitting = 1 << 2,
        MobSaddled = 1 << 3,
        MobCharged = 1 << 4,
        MobCaptain = 1 << 5,
        MobBurning = 1 << 6,
        MobHurt = 1 << 7,
        MobLeashed = 1 << 8
    };

    struct MobNoise
    {
        uint16_t sound = 0;
        std::string named;
        glm::vec3 position{ 0.0f };
        float volume = 1.0f;
        float pitch = 1.0f;
        uint16_t mob = 0;
    };

    struct MobHitRequest
    {
        uint32_t peer = 0;
        uint32_t serial = 0;
        int damage = 0;
        glm::vec3 push{ 0.0f };
        float fireSeconds = 0.0f;
    };

    struct StrikeOnYou
    {
        int damage = 0;
        glm::vec3 from{ 0.0f };
        int blindSeconds = 0;
        int effect = -1;
        int effectAmplifier = 0;
        float effectSeconds = 0.0f;
        int attacker = -1;
    };

    // What a guest needs before it can build the world at all.
    struct Handshake
    {
        bool received = false;
        uint32_t seed = 0;
        uint8_t gameMode = 0;
        float timeOfDay = 0.3f;
        glm::vec3 spawn{ 0.0f };
        std::string generator;
    };

    // The multiplayer half of "Open to LAN".
    //
    // The host is authoritative, but only just: guests simulate their own
    // movement and apply their own edits immediately, then tell the host,
    // which confirms by echoing the change back to everyone. That keeps
    // the game responsive without a prediction/rollback system, and the
    // worst case for a disagreement is one block flickering.
    class Session
    {
    public:
        Session();
        ~Session();

        // Share the world that is already running.
        bool openToLan(const std::string& playerName, uint32_t seed,
                       uint8_t gameMode, float timeOfDay, const glm::vec3& spawn,
                       uint16_t port = DEFAULT_PORT);
        bool join(const std::string& address, uint16_t port, const std::string& playerName);
        void leave();

        // Which character the local player is wearing. Set before opening
        // or joining a world, and again whenever it is changed on the
        // profile screen; it is read at handshake time.
        void setLocalAppearance(const Appearance& look) { m_localLook = look; }

        // Posture that cannot be worked out from a position alone.
        void setLocalSneaking(bool sneaking) { m_localSneaking = sneaking; }
        void setLocalHands(uint16_t mainHand, uint16_t offHand, bool leftHanded)
        {
            m_localMain = mainHand;
            m_localOff = offHand;
            m_localLeftHanded = leftHanded;
        }
        void noteSwing(bool offHand)
        {
            ++m_localSwings;
            m_localSwingOff = offHand;
        }
        static constexpr float SWING_SECONDS = 0.30f;

        void broadcastMobs(const std::vector<MobSnap>& mobs);
        void broadcastMobNoise(const MobNoise& noise);
        void strikeGuest(uint32_t peer, const StrikeOnYou& strike);
        void sendMobHit(uint32_t serial, int damage, const glm::vec3& push, float fireSeconds);
        bool takeMobStates(std::vector<MobSnap>& out);
        std::vector<MobNoise> takeMobNoises() { std::vector<MobNoise> all; all.swap(m_mobNoises); return all; }
        std::vector<StrikeOnYou> takeStrikesOnMe() { std::vector<StrikeOnYou> all; all.swap(m_strikesOnMe); return all; }
        std::vector<MobHitRequest> takeMobHits() { std::vector<MobHitRequest> all; all.swap(m_mobHits); return all; }

        // Pumped once a frame. `world` is null until a guest has built it.
        void update(float deltaTime, World* world, const glm::vec3& localPosition,
                    float yaw, float pitch, float& timeOfDay);

        // The local player changed a block. On the host that is final and
        // gets broadcast; on a guest it is a request that has already been
        // applied locally. Safe to call when offline.
        void onLocalBlockChange(int x, int y, int z, uint16_t block);

        Role role() const { return m_role; }
        Status status() const { return m_status; }
        bool active() const { return m_role != Role::Offline; }
        const Handshake& handshake() const { return m_handshake; }
        void setGenerator(const std::string& generator) { m_handshake.generator = generator; }
        void setMotd(const std::string& motd) { m_motd = motd; }
        const std::string& message() const { return m_message; }
        uint16_t port() const { return m_port; }

        const std::unordered_map<uint32_t, RemotePlayer>& players() const { return m_players; }
        int playerCount() const { return static_cast<int>(m_players.size()) + 1; }
        bool kick(const std::string& name, const std::string& reason);
        std::set<std::string> banned;
        std::set<std::string> whitelist;
        bool whitelistOn = false;

    private:
        std::unique_ptr<Transport> m_transport;
        Role m_role = Role::Offline;
        Status m_status = Status::Idle;
        std::string m_message;
        std::string m_localName;
        std::string m_motd;
        uint16_t m_port = DEFAULT_PORT;

        Handshake m_handshake;
        uint32_t m_localId = 0;
        Appearance m_localLook;
        bool m_localSneaking = false;
        std::vector<MobSnap> m_mobStates;
        bool m_mobStatesFresh = false;
        std::vector<MobNoise> m_mobNoises;
        std::vector<StrikeOnYou> m_strikesOnMe;
        std::vector<MobHitRequest> m_mobHits;
        uint16_t m_localMain = 0;
        uint16_t m_localOff = 0;
        bool m_localLeftHanded = false;
        uint8_t m_localSwings = 0;
        bool m_localSwingOff = false;
        uint16_t m_lastSentMain = 0;
        uint16_t m_lastSentOff = 0;
        uint8_t m_lastSentSwings = 0;
        bool m_lastSentLeft = false;

        // Host-side copy of what every guest was told, so a late joiner
        // can be introduced to everyone already here.
        std::unordered_map<uint32_t, RemotePlayer> m_players;

        // Edits from the host that could not be applied yet because the
        // chunk they belong to has not streamed in. Retried until it has.
        struct PendingEdit { int x, y, z; uint16_t block; };
        std::vector<PendingEdit> m_pending;
        float m_retryTimer = 0.0f;

        bool m_timeSynced = false;  // a TimeSync landed; adopt it this frame
        float m_moveTimer = 0.0f;
        float m_timeSyncTimer = 0.0f;
        glm::vec3 m_lastSentPosition{ 0.0f };
        float m_lastSentYaw = 0.0f;
        float m_lastSentPitch = 0.0f;
        bool m_lastSentSneak = false;
        // Send the next movement packet whether or not anything moved,
        // so a player who joins while we stand still still learns where
        // we are.
        bool m_forceMove = false;

        // Scratch, reused every poll so the frame does no allocation.
        std::vector<Packet> m_incoming;
        std::vector<PeerId> m_joined;
        std::vector<PeerId> m_left;

        void handleAsHost(const Packet& packet, World* world);
        void handleAsGuest(const Packet& packet, World* world);
        void sendWorldDeltas(PeerId peer, World& world);
        void applyEdit(World* world, int x, int y, int z, uint16_t block);
        void retryPending(World* world, float deltaTime);
        void sendMovement(float deltaTime, const glm::vec3& position, float yaw, float pitch);
    };
}
