#include "Session.h"
#include "World/World.h"
#include "World/WorldSave.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>

#ifndef __EMSCRIPTEN__
#include "SocketTransport.h"
#endif

namespace Net
{
namespace
{
    constexpr float MOVE_INTERVAL = 1.0f / 20.0f;   // 20 position updates a second
    constexpr float TIME_SYNC_INTERVAL = 10.0f;
    constexpr float RETRY_INTERVAL = 1.0f;
    constexpr size_t EDITS_PER_MESSAGE = 2048;
    constexpr size_t MAX_PENDING_EDITS = 200000;

    // Below these the move is not worth a packet.
    constexpr float MOVE_EPSILON = 0.02f;
    constexpr float LOOK_EPSILON = 0.8f;
}

Session::Session() = default;
Session::~Session() { leave(); }

// ---------------------------------------------------------- lifecycle ---

bool Session::openToLan(const std::string& playerName, uint32_t seed, uint8_t gameMode,
                        float timeOfDay, const glm::vec3& spawn, uint16_t port)
{
#ifdef __EMSCRIPTEN__
    (void)playerName; (void)seed; (void)gameMode; (void)timeOfDay; (void)spawn; (void)port;
    m_status = Status::Failed;
    m_message = "Multiplayer in the browser is not wired up yet";
    return false;
#else
    leave();

    auto transport = std::make_unique<SocketTransport>();
    if (!transport->listen(port))
    {
        m_status = Status::Failed;
        m_message = transport->error();
        return false;
    }

    m_transport = std::move(transport);
    m_role = Role::Host;
    m_status = Status::Playing;
    m_localName = playerName;
    m_localId = HOST_PEER;
    m_port = port;

    m_handshake.received = true;
    m_handshake.seed = seed;
    m_handshake.gameMode = gameMode;
    m_handshake.timeOfDay = timeOfDay;
    m_handshake.spawn = spawn;

    m_message = "Open on port " + std::to_string(port);
    return true;
#endif
}

bool Session::join(const std::string& address, uint16_t port, const std::string& playerName)
{
#ifdef __EMSCRIPTEN__
    (void)address; (void)port; (void)playerName;
    m_status = Status::Failed;
    m_message = "Multiplayer in the browser is not wired up yet";
    return false;
#else
    leave();

    auto transport = std::make_unique<SocketTransport>();
    if (!transport->connect(address, port))
    {
        m_status = Status::Failed;
        m_message = transport->error();
        return false;
    }

    m_transport = std::move(transport);
    m_role = Role::Guest;
    m_status = Status::Connecting;
    m_localName = playerName;
    m_port = port;
    m_message = "Connecting to " + address;
    return true;
#endif
}

void Session::leave()
{
    if (m_transport) m_transport->close();
    m_transport.reset();
    m_role = Role::Offline;
    m_status = Status::Idle;
    m_players.clear();
    m_pending.clear();
    m_handshake = Handshake{};
    m_localId = 0;
    m_message.clear();

    // Forget what the last session had already told everyone, so the
    // first packet of the next one goes out even if nothing has moved
    // since -- otherwise rejoining from the same spot leaves you
    // invisible until you take a step.
    m_lastSentPosition = glm::vec3(0.0f);
    m_lastSentYaw = 0.0f;
    m_lastSentPitch = 0.0f;
    m_lastSentSneak = false;
    m_forceMove = true;
}

// ------------------------------------------------------------- update ---

void Session::update(float deltaTime, World* world, const glm::vec3& localPosition,
                     float yaw, float pitch, float& timeOfDay)
{
    if (!m_transport) return;

    m_transport->poll(m_incoming, m_joined, m_left);

    for (PeerId peer : m_joined)
    {
        if (m_role == Role::Guest)
        {
            // The socket is up; introduce ourselves.
            Writer hello(MessageId::Hello);
            hello.u32(PROTOCOL_VERSION);
            hello.str(m_localName);
            hello.u8(m_localLook.variant);
            hello.u8(m_localLook.slim ? 1 : 0);
            m_transport->send(HOST_PEER, hello.bytes());
            m_status = Status::Loading;
            m_message = "Loading world";
        }
        else
        {
            // Nothing to do until they say Hello -- an open port attracts
            // port scanners, and they never introduce themselves.
            (void)peer;
        }
    }

    for (const Packet& packet : m_incoming)
    {
        if (m_role == Role::Host) handleAsHost(packet, world);
        else handleAsGuest(packet, world);
    }

    for (PeerId peer : m_left)
    {
        if (m_role == Role::Guest)
        {
            m_status = Status::Failed;
            if (m_message.empty() || m_status != Status::Failed)
                m_message = "Disconnected from the host";
            m_message = m_transport->error().empty() ? "Disconnected from the host"
                                                     : m_transport->error();
            continue;
        }

        auto it = m_players.find(peer);
        if (it == m_players.end()) continue;
        std::printf("[net] %s left\n", it->second.name.c_str());
        std::fflush(stdout);
        m_players.erase(it);

        Writer bye(MessageId::PlayerLeave);
        bye.u32(peer);
        m_transport->broadcast(bye.bytes());
    }

    // Blend remote players towards wherever they last said they were.
    for (auto& entry : m_players)
    {
        RemotePlayer& player = entry.second;
        player.blend = std::min(1.0f, player.blend + deltaTime / MOVE_INTERVAL);
        player.silence += deltaTime;
    }

    if (m_timeSynced)
    {
        timeOfDay = m_handshake.timeOfDay;
        m_timeSynced = false;
    }

    retryPending(world, deltaTime);
    sendMovement(deltaTime, localPosition, yaw, pitch);

    // The host's clock is the world's clock.
    if (m_role == Role::Host)
    {
        m_timeSyncTimer += deltaTime;
        if (m_timeSyncTimer >= TIME_SYNC_INTERVAL && !m_players.empty())
        {
            m_timeSyncTimer = 0.0f;
            Writer sync(MessageId::TimeSync);
            sync.f32(timeOfDay);
            m_transport->broadcast(sync.bytes());
        }
    }
}

void Session::sendMovement(float deltaTime, const glm::vec3& position, float yaw, float pitch)
{
    if (m_status != Status::Playing || m_players.empty()) return;

    m_moveTimer += deltaTime;
    if (m_moveTimer < MOVE_INTERVAL) return;
    m_moveTimer = 0.0f;

    // Standing still costs nothing -- but dropping into a sneak while
    // standing still is still news, because it changes how you are drawn,
    // and so is somebody new arriving who has never been told where we
    // are at all.
    if (!m_forceMove &&
        glm::length(position - m_lastSentPosition) < MOVE_EPSILON &&
        std::abs(yaw - m_lastSentYaw) < LOOK_EPSILON &&
        std::abs(pitch - m_lastSentPitch) < LOOK_EPSILON &&
        m_localSneaking == m_lastSentSneak)
        return;

    m_forceMove = false;

    m_lastSentPosition = position;
    m_lastSentYaw = yaw;
    m_lastSentPitch = pitch;
    m_lastSentSneak = m_localSneaking;

    const uint8_t flags = m_localSneaking ? static_cast<uint8_t>(MoveSneaking) : uint8_t{ 0 };

    if (m_role == Role::Host)
    {
        Writer move(MessageId::PlayerMove);
        move.u32(m_localId);
        move.vec3(position);
        move.f32(yaw);
        move.f32(pitch);
        move.u8(flags);
        m_transport->broadcast(move.bytes());
    }
    else
    {
        Writer move(MessageId::Move);
        move.vec3(position);
        move.f32(yaw);
        move.f32(pitch);
        move.u8(flags);
        m_transport->send(HOST_PEER, move.bytes());
    }
}

// ------------------------------------------------------------- blocks ---

void Session::onLocalBlockChange(int x, int y, int z, uint16_t block)
{
    if (!m_transport || m_status == Status::Idle) return;

    if (m_role == Role::Host)
    {
        Writer change(MessageId::BlockChange);
        change.i32(x); change.i32(y); change.i32(z);
        change.u16(block);
        m_transport->broadcast(change.bytes());
    }
    else if (m_status == Status::Playing)
    {
        Writer request(MessageId::RequestBlock);
        request.i32(x); request.i32(y); request.i32(z);
        request.u16(block);
        m_transport->send(HOST_PEER, request.bytes());
    }
}

void Session::applyEdit(World* world, int x, int y, int z, uint16_t block)
{
    // A guest can be told about a block long before that chunk has
    // streamed in, so anything that does not stick is kept and retried.
    if (world && world->chunkAt(World::floorDiv(x, Chunk::SX), World::floorDiv(z, Chunk::SZ)))
    {
        world->setBlock(x, y, z, static_cast<BlockId>(block));
        return;
    }

    if (m_pending.size() < MAX_PENDING_EDITS)
        m_pending.push_back({ x, y, z, block });
}

void Session::retryPending(World* world, float deltaTime)
{
    if (m_pending.empty() || !world) return;

    m_retryTimer += deltaTime;
    if (m_retryTimer < RETRY_INTERVAL) return;
    m_retryTimer = 0.0f;

    std::vector<PendingEdit> stillPending;
    stillPending.reserve(m_pending.size());

    for (const PendingEdit& edit : m_pending)
    {
        if (world->chunkAt(World::floorDiv(edit.x, Chunk::SX), World::floorDiv(edit.z, Chunk::SZ)))
            world->setBlock(edit.x, edit.y, edit.z, static_cast<BlockId>(edit.block));
        else
            stillPending.push_back(edit);
    }

    m_pending.swap(stillPending);
}

// Everything the host has changed since the world was generated. The save
// format already stores exactly the chunks the player touched, so the
// honest diff is: regenerate each of those from the seed and compare.
void Session::sendWorldDeltas(PeerId peer, World& world)
{
    std::vector<PendingEdit> edits;
    std::vector<ChunkPos> positions;

    for (const auto& entry : world.chunks())
        if (entry.second && entry.second->modified) positions.push_back(entry.first);

    // Chunks the host edited earlier and has since walked away from only
    // exist on disk now.
    std::error_code ec;
    const std::string chunkDir = world.saveDirectory() + "/chunks";
    if (std::filesystem::exists(chunkDir, ec))
    {
        for (const auto& file : std::filesystem::directory_iterator(chunkDir, ec))
        {
            int cx = 0, cz = 0;
            const std::string name = file.path().filename().string();
            if (std::sscanf(name.c_str(), "c.%d.%d.dat", &cx, &cz) != 2) continue;

            const ChunkPos position{ cx, cz };
            if (std::find(positions.begin(), positions.end(), position) == positions.end())
                positions.push_back(position);
        }
    }

    for (const ChunkPos& position : positions)
    {
        // The live chunk wins: it has edits that may not be saved yet.
        Chunk* live = world.chunkAt(position.x, position.z);
        Chunk fromDisk(position);
        const Chunk* actual = live;
        if (!actual)
        {
            if (!WorldSave::loadChunk(world.saveDirectory(), fromDisk)) continue;
            actual = &fromDisk;
        }

        Chunk pristine(position);
        world.generator().generate(pristine);

        for (int y = 0; y < Chunk::SY; ++y)
            for (int z = 0; z < Chunk::SZ; ++z)
                for (int x = 0; x < Chunk::SX; ++x)
                {
                    const BlockId was = pristine.getBlock(x, y, z);
                    const BlockId is = actual->getBlock(x, y, z);
                    if (was == is) continue;
                    edits.push_back({ position.x * Chunk::SX + x, y,
                                      position.z * Chunk::SZ + z, static_cast<uint16_t>(is) });
                }
    }

    for (size_t start = 0; start < edits.size(); start += EDITS_PER_MESSAGE)
    {
        const size_t count = std::min(EDITS_PER_MESSAGE, edits.size() - start);
        Writer delta(MessageId::ChunkDelta);
        delta.u32(static_cast<uint32_t>(count));
        for (size_t i = 0; i < count; ++i)
        {
            const PendingEdit& edit = edits[start + i];
            delta.i32(edit.x); delta.i32(edit.y); delta.i32(edit.z);
            delta.u16(edit.block);
        }
        m_transport->send(peer, delta.bytes());
    }

    Writer done(MessageId::DeltaComplete);
    done.u32(static_cast<uint32_t>(edits.size()));
    m_transport->send(peer, done.bytes());
}

// --------------------------------------------------------------- host ---

void Session::handleAsHost(const Packet& packet, World* world)
{
    Reader reader(packet.bytes.data(), packet.bytes.size());
    const MessageId id = reader.id();

    switch (id)
    {
        case MessageId::Hello:
        {
            // The version is read and answered before anything else: an
            // older client's Hello is shorter than this one, so reading
            // the rest first would fail and they would hang on a silent
            // socket instead of being told why they were turned away.
            const uint32_t version = reader.u32();
            if (!reader.ok() || version != PROTOCOL_VERSION)
            {
                Writer reject(MessageId::Reject);
                reject.str("That copy of Browsercraft is a different version");
                m_transport->send(packet.peer, reject.bytes());
                m_transport->disconnect(packet.peer);
                return;
            }

            std::string name = reader.str();
            Appearance look;
            look.variant = reader.u8();
            look.slim = reader.u8() != 0;
            if (!reader.ok()) return;

            if (name.empty()) name = "PLAYER";

            Writer welcome(MessageId::Welcome);
            welcome.u32(PROTOCOL_VERSION);
            welcome.u32(packet.peer);          // the id they will be known by
            welcome.u32(m_handshake.seed);
            welcome.u8(m_handshake.gameMode);
            welcome.f32(m_handshake.timeOfDay);
            welcome.vec3(m_handshake.spawn);
            welcome.str(m_localName);          // the host, so they can label us
            welcome.u8(m_localLook.variant);
            welcome.u8(m_localLook.slim ? 1 : 0);
            m_transport->send(packet.peer, welcome.bytes());

            // Introduce everyone already here, then everyone to them.
            // Each introduction carries that player's last known position
            // too: a player who has been standing still since before this
            // one arrived sends nothing of their own, and would be
            // invisible to the newcomer until they next moved.
            for (const auto& entry : m_players)
            {
                Writer join(MessageId::PlayerJoin);
                join.u32(entry.first);
                join.str(entry.second.name);
                join.u8(entry.second.look.variant);
                join.u8(entry.second.look.slim ? 1 : 0);
                m_transport->send(packet.peer, join.bytes());

                if (!entry.second.positioned) continue;

                Writer where(MessageId::PlayerMove);
                where.u32(entry.first);
                where.vec3(entry.second.position);
                where.f32(entry.second.yaw);
                where.f32(entry.second.pitch);
                where.u8(entry.second.sneaking ? static_cast<uint8_t>(MoveSneaking)
                                               : uint8_t{ 0 });
                m_transport->send(packet.peer, where.bytes());
            }

            // ...and the host is a player too, standing still or not.
            m_forceMove = true;

            Writer announce(MessageId::PlayerJoin);
            announce.u32(packet.peer);
            announce.str(name);
            announce.u8(look.variant);
            announce.u8(look.slim ? 1 : 0);
            m_transport->broadcast(announce.bytes(), packet.peer);

            RemotePlayer player;
            player.id = packet.peer;
            player.name = name;
            player.look = look;
            m_players[packet.peer] = player;

            std::printf("[net] %s joined as player %u\n", name.c_str(), packet.peer);
            std::fflush(stdout);

            if (world) sendWorldDeltas(packet.peer, *world);
            else
            {
                Writer done(MessageId::DeltaComplete);
                done.u32(0);
                m_transport->send(packet.peer, done.bytes());
            }
            break;
        }

        case MessageId::RequestBlock:
        {
            const int x = reader.i32(), y = reader.i32(), z = reader.i32();
            const uint16_t block = reader.u16();
            if (!reader.ok() || !world) return;

            // The host is the one that decides, so apply it here and echo
            // it to everyone -- including the sender, which is how they
            // find out if their optimistic edit was wrong.
            world->setBlock(x, y, z, static_cast<BlockId>(block));

            Writer change(MessageId::BlockChange);
            change.i32(x); change.i32(y); change.i32(z);
            change.u16(block);
            m_transport->broadcast(change.bytes());
            break;
        }

        case MessageId::Move:
        {
            const glm::vec3 position = reader.vec3();
            const float yaw = reader.f32();
            const float pitch = reader.f32();
            const uint8_t flags = reader.u8();
            if (!reader.ok()) return;

            auto it = m_players.find(packet.peer);
            if (it == m_players.end()) return;

            // A first position, or one after a long silence, is where
            // they are rather than somewhere to glide towards.
            it->second.previous = (!it->second.positioned || it->second.silence > 2.0f)
                                      ? position : it->second.smoothed();
            it->second.position = position;
            it->second.yaw = yaw;
            it->second.pitch = pitch;
            it->second.sneaking = (flags & MoveSneaking) != 0;
            it->second.blend = 0.0f;
            it->second.silence = 0.0f;
            it->second.positioned = true;

            Writer move(MessageId::PlayerMove);
            move.u32(packet.peer);
            move.vec3(position);
            move.f32(yaw);
            move.f32(pitch);
            move.u8(flags);
            m_transport->broadcast(move.bytes(), packet.peer);
            break;
        }

        default:
            break;
    }
}

// -------------------------------------------------------------- guest ---

void Session::handleAsGuest(const Packet& packet, World* world)
{
    Reader reader(packet.bytes.data(), packet.bytes.size());
    const MessageId id = reader.id();

    switch (id)
    {
        case MessageId::Welcome:
        {
            const uint32_t version = reader.u32();
            m_localId = reader.u32();
            m_handshake.seed = reader.u32();
            m_handshake.gameMode = reader.u8();
            m_handshake.timeOfDay = reader.f32();
            m_handshake.spawn = reader.vec3();
            const std::string hostName = reader.str();
            Appearance hostLook;
            hostLook.variant = reader.u8();
            hostLook.slim = reader.u8() != 0;

            if (version != PROTOCOL_VERSION || !reader.ok())
            {
                m_status = Status::Failed;
                m_message = "That world is running a different version";
                return;
            }

            // The host is a player too, and always has id 0.
            RemotePlayer host;
            host.id = HOST_PEER;
            host.name = hostName.empty() ? "HOST" : hostName;
            host.look = hostLook;
            host.position = m_handshake.spawn;
            host.previous = m_handshake.spawn;
            m_players[HOST_PEER] = host;

            m_handshake.received = true;
            m_status = Status::Loading;
            m_message = "Building the world";
            break;
        }

        case MessageId::Reject:
            m_message = reader.str();
            m_status = Status::Failed;
            break;

        case MessageId::ChunkDelta:
        {
            const uint32_t count = reader.u32();
            for (uint32_t i = 0; i < count && reader.ok(); ++i)
            {
                const int x = reader.i32(), y = reader.i32(), z = reader.i32();
                const uint16_t block = reader.u16();
                if (reader.ok()) applyEdit(world, x, y, z, block);
            }
            break;
        }

        case MessageId::DeltaComplete:
            m_status = Status::Playing;
            m_message.clear();
            std::printf("[net] joined; %u blocks of changes received\n", reader.u32());
            std::fflush(stdout);
            break;

        case MessageId::BlockChange:
        {
            const int x = reader.i32(), y = reader.i32(), z = reader.i32();
            const uint16_t block = reader.u16();
            if (reader.ok()) applyEdit(world, x, y, z, block);
            break;
        }

        case MessageId::PlayerJoin:
        {
            const uint32_t who = reader.u32();
            const std::string name = reader.str();
            Appearance look;
            look.variant = reader.u8();
            look.slim = reader.u8() != 0;
            if (!reader.ok() || who == m_localId) return;

            RemotePlayer player;
            player.id = who;
            player.name = name;
            player.look = look;
            m_players[who] = player;
            break;
        }

        case MessageId::PlayerLeave:
        {
            const uint32_t who = reader.u32();
            if (reader.ok()) m_players.erase(who);
            break;
        }

        case MessageId::PlayerMove:
        {
            const uint32_t who = reader.u32();
            const glm::vec3 position = reader.vec3();
            const float yaw = reader.f32();
            const float pitch = reader.f32();
            const uint8_t flags = reader.u8();
            if (!reader.ok() || who == m_localId) return;

            RemotePlayer& player = m_players[who];
            player.id = who;
            player.previous = (!player.positioned || player.silence > 2.0f)
                                  ? position : player.smoothed();
            player.position = position;
            player.yaw = yaw;
            player.pitch = pitch;
            player.sneaking = (flags & MoveSneaking) != 0;
            player.blend = 0.0f;
            player.silence = 0.0f;
            player.positioned = true;
            break;
        }

        case MessageId::TimeSync:
            m_handshake.timeOfDay = reader.f32();
            m_timeSynced = true;
            break;

        default:
            break;
    }
}

} // namespace Net
