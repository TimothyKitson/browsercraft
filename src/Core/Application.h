#pragma once
#include <array>
#include <set>
#include "Window.h"
#include "Input.h"
#include "KeyBindings.h"
#include "Renderer/Shader.h"
#include "Renderer/Atlas.h"
#include "Renderer/Camera.h"
#include "Renderer/UIRenderer.h"
#include "Renderer/SkyRenderer.h"
#include "Renderer/ShaderPipeline.h"
#include "Renderer/SelectionRenderer.h"
#include "Renderer/GuiTextures.h"
#include "Entity/SpriteParticles.h"
#include "World/World.h"
#include "World/Furnaces.h"
#include "World/Chests.h"
#include "Entity/Projectiles.h"
#include "Player/Player.h"
#include "Player/Inventory.h"
#include "Audio/AudioEngine.h"
#include "Entity/DroppedItems.h"
#include "Entity/ExperienceOrbs.h"
#include "Entity/Rain.h"
#include "Game/Anvil.h"
#include "Game/Raids.h"
#include "Game/RecipeBook.h"
#include "Game/Advancements.h"
#include "Game/Stats.h"
#include "Game/Worlds.h"
#include "Game/Servers.h"
#include "Net/Ping.h"
#include "Core/Packs.h"
#include "Game/Brewing.h"
#include "World/Signs.h"
#include "World/Frames.h"
#include "Game/Shield.h"
#include "Game/Fluids.h"
#include "Renderer/BeamRenderer.h"
#include "Renderer/BorderRenderer.h"
#include "World/WorldBorder.h"
#include "Renderer/BookRenderer.h"
#include "Renderer/FangRenderer.h"
#include "Renderer/PaintingRenderer.h"
#include "Renderer/ArmorStandRenderer.h"
#include "Renderer/SignRenderer.h"
#include "Renderer/ArmourTextures.h"
#include "Renderer/BannerRenderer.h"
#include "Game/Banners.h"
#include "Game/Fireworks.h"
#include "Game/Discs.h"
#include "Renderer/FireworkRenderer.h"
#include "Renderer/ShadowRenderer.h"
#include "Game/VillageLoot.h"
#include "Game/StructureLoot.h"
#include "Game/Weather.h"
#include "Game/Trading.h"
#include "Entity/Particles.h"
#include "Entity/PlayerSkin.h"
#include "Entity/PlayerModel.h"
#include "Entity/HeldItem.h"
#include "Game/Chat.h"
#include "Game/Beacon.h"
#include "Game/Grindstone.h"
#include "Game/Saplings.h"
#include "Game/Hopper.h"
#include "Entity/EntityManager.h"
#include "Entity/MobModel.h"
#include "Game/GameMode.h"
#include "Net/Session.h"
#ifndef __EMSCRIPTEN__
#include "Net/SocketTransport.h"   // LAN announce/discover; desktop only
#endif
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <map>
#include <functional>
#include "Game/Maps.h"
#include "Game/Sculk.h"
#include "Game/Fishing.h"

// Which part of the game is on screen. The world only exists while playing,
// so the title screen can run before anything has been generated.
enum class Screen
{
    NameEntry,   // "profile": your name, your character, your model
    Title,
    Singleplayer,
    Multiplayer,
    Settings,
    VideoSettings,
    Credits,
    Controls,
    Playing,
    Paused,
    Dead,
    Statistics,
    CreateWorld,
    EditWorld,
    ConfirmDelete,
    CustomizeFlat,
    FlatPresets,
    CustomizeBiome,
    EditServer,
    DirectConnect,
    Connecting,
    Packs,
    Accessibility,
    Language,
    LockDifficulty
};

// Menu buttons are drawn and hit-tested in the same pass, so a click
// records what to do and the loop carries it out once drawing is finished.
// Starting a world mid-render would pull the ground out from under the
// very UI being drawn.
enum class MenuAction
{
    None,
    GoSingleplayer,
    ToggleCheats,
    GoMultiplayer,
    GoSettings,
    GoControls,
    ResetBindings,
    GoNameEntry,
    ConfirmName,
    PickSkin,
    ToggleSkinStyle,
    OpenToLan,
    CloseToLan,
    JoinTyped,
    JoinDiscovered,
    ResumeSaved,
    NewSurvival,
    NewCreative,
    NewHardcore,
    BackToTitle,
    Resume,
    QuitToTitle,
    QuitGame,
    Respawn,
    DeleteWorld,
    TogglePbr,
    ToggleShaders,
    GoVideoSettings,
    GoCredits,
    BackToOptions,
    LeaveOptions,
    ToggleSound,
    CycleAttackIndicator,
    CycleDistortion,
    OpenAdvancements,
    OpenStatistics,
    PlayWorld,
    OpenPacks,
    OpenAccessibility,
    OpenLanguage,
    CreateWorldNow,
    ToggleMainHand,
    RenderDistanceDown,
    SetRenderDistance,
    RenderDistanceUp
};

// Owns every subsystem and runs the game loop. Read frame() first: it shows
// the shape of a frame (input -> simulate -> stream -> render).
class Application
{
public:
    struct BookText
    {
        std::string title;
        std::string author;
        std::vector<std::string> pages;
    };

    Application(int windowWidth, int windowHeight);
    ~Application();

    void run();

    // Takes a screenshot this many seconds after the world is ready, then
    // quits. Used for automated visual checks; 0 disables it.
    void setAutoScreenshot(float delaySeconds) { m_autoScreenshotDelay = delaySeconds; }
    void setPbrEnabled(bool enabled) { m_pbrEnabled = enabled; }
    void setShadersEnabled(bool enabled) { m_shadersOn = enabled; }
    void setTimeOverride(float timeOfDay) { m_timeOverride = timeOfDay; }
    // Skips the title screen and drops straight into a world.
    void setStartupMode(GameMode mode) { m_startupMode = mode; m_startImmediately = true; }
    // Dev aid: open on a particular screen so menus can be screenshotted.
    void setStartupScreen(const std::string& screen) { m_startupScreen = screen; }
    // Generate the next new world from a specific seed instead of a random one.
    void setSeedOverride(uint32_t seed) { m_seedOverride = seed; m_hasSeedOverride = true; }
    // Dev aids: start a world and immediately share it, or join one.
    void setHostOnStart() { m_hostOnStart = true; m_startImmediately = true; }
    void setJoinOnStart(const std::string& address) { m_joinOnStart = address; }
    // Dev aid: stamp a landmark at spawn, so an automated run has
    // something to check block syncing against.
    void setBuildTest() { m_buildTest = true; }
    // Dev aid: line one of every species up in front of the spawn point,
    // so a screenshot shows all eight without waiting for the spawner.
    // The line-up is for looking at hides, so nothing in it moves:
    // frozen, every one of them is still there when the shutter goes,
    // and the shulker -- which shoots from sixteen blocks -- does not
    // kill the photographer before the shot is taken.
    void setMobTest() { m_mobTest = true; m_startImmediately = true; m_freezeMobs = true; }
    // Dev aid: sow a field of wheat at every stage of growth in front
    // of the player, so a screenshot shows the whole crop at once.
    void setFarmTest() { m_farmTest = true; m_startImmediately = true; }
    void setShopTest() { m_shopTest = true; m_startImmediately = true; }
    // Dev aid: a tank of water with the three things that live in one
    // put into it, which is the only way to see whether they can be.
    void setSwimTest() { m_swimTest = true; m_startImmediately = true; }
    // Dev aid: start at the nearest structure of a named kind rather
    // than at the spawn the generator would pick. A structure can be
    // a thousand blocks from spawn, and walking there is not a check
    // anybody runs twice.
    void setStartAt(const std::string& landmark)
    {
        m_startAt = landmark;
        m_startImmediately = true;
        // In creative, because the point of standing at an outpost is
        // to look at it. The first survival run of this ended in a
        // death screen before the shutter, which did at least prove
        // the pillagers were awake.
        m_startupMode = GameMode::Creative;
    }
    // Dev aid: open straight into another dimension.
    void setStartupDimension(Dimension dimension) { m_startupDimension = dimension; }
    // Dev aid: preview a candidate panorama viewpoint.
    void setPanoramaOverride(uint32_t seed, float x, float y, float z)
    {
        m_panoramaSeed = seed;
        m_panoramaSpot = glm::vec3(x, y, z);
        m_hasPanoramaOverride = true;
    }

private:
    Window m_window;
    Input m_input;
    KeyBindings m_keys;

    // Which control the player is currently rebinding, and the one the
    // mouse is over. Set while the controls screen is open.
    Action m_rebinding = Action::Count;
    int m_controlsHover = -1;
    Atlas m_atlas;
    GuiTextures m_gui;
    Shader m_chunkShader;
    SkyRenderer m_sky;
    std::unique_ptr<ShaderPipeline> m_fx;
    bool m_shadersOn = false;
    Shader* m_activeShader = nullptr;
    Screen m_optionsReturn = Screen::Title;
    bool shadersActive();
    void renderWorldShaded();
    void renderVideoScreen();
    void renderCreditsScreen();
    void loadOptions();
    void saveOptions();
    SelectionRenderer m_selection;
    UIRenderer m_ui;

    AudioEngine m_audio;
    // Only alive while a world is loaded, so the title screen costs nothing.
    std::unique_ptr<World> m_world;
    Camera m_camera;
    Player m_player;
    Inventory m_inventory;
    DroppedItems m_drops;
    ExperienceOrbs m_orbs;

    // The weather, and what you see of it. Only the overworld has any:
    // there is no sky over the Nether to rain out of, and the End has
    // no weather in Minecraft either.
    Weather::State m_weather;
    uint32_t m_weatherSeed = 1u;
    Rain m_rain;
    float m_rainSoundTimer = 0.0f;

    // True where the player is standing: rain turns to snow in the cold
    // biomes, the same as it does in Minecraft.
    bool m_snowing = false;

    void updateWeather(float deltaTime);

    // What the open furnace's output slot held last frame. Smelting is
    // paid for when the item leaves the slot, as in Minecraft, and
    // watching the count fall catches every route out of it -- click,
    // shift-click or drag -- without each needing its own hook.
    StackId m_furnaceOutputId = Blocks::Air;
    int m_furnaceOutputCount = 0;
    Particles m_particles;
    std::unique_ptr<SpriteParticles> m_sprites;
    float m_ambientClock = 0.0f;
    float m_bubbleClock = 0.0f;
    struct TimedBlock { glm::ivec3 at; BlockId expect; BlockId then; float left; };
    std::vector<TimedBlock> m_timedBlocks;
    std::vector<std::pair<glm::ivec3, float>> m_tiltingLeaves;
    std::vector<glm::vec3> m_boltPoints;
    float m_boltLeft = 0.0f;
    void strikeLightning();
    void setCopper(const glm::ivec3& at, BlockId now);
    Fluids::Scheduler m_fluids;
    std::vector<glm::ivec3> m_fluidChanges;
    const World* m_fluidWorld = nullptr;
    float m_fluidClock = 0.0f;
    long long m_fluidTicks = 0;
    void tickFluids(float deltaTime);
    void tickHives(float deltaTime);
    void claimJobSites();
    float m_horseJumpHeld = 0.0f;
    bool m_horseOpen = false;
    uint32_t m_horseSeed = 0;
    void openHorse(Mob& horse);
    void stowHorse();
    float m_horseJumpCooldown = 0.0f;
    float horseJumpStrength() const;
    BeamRenderer m_beams;
    BorderRenderer m_borderRenderer;
    WorldBorder m_border;
    void renderBorder(const glm::mat4& view, const glm::mat4& projection);
    void keepInsideBorder(const glm::vec3& before);
    glm::vec3 m_biomeSky{ 1.0f };
    glm::vec3 m_biomeFog{ 0.30f, 0.09f, 0.07f };
    int m_biomeColourDimension = -1;
    void updateBiomeColours(float deltaTime);
    BookRenderer m_books;
    BannerRenderer m_bannerRenderer;
    ShadowRenderer m_shadows;
    void castShadows(const glm::mat4& view, const glm::mat4& projection);
    float m_beamClock = 0.0f;
    float m_dragonBreathTimer = 5.0f;
    std::vector<glm::ivec3> m_litBeacons;
    std::vector<std::pair<glm::ivec3, float>> m_gatewayFlashes;
    struct BreathCloud { glm::vec3 at; float left; float tick; };
    std::vector<BreathCloud> m_breathClouds;
    void queueBeams();
    void tickBreathClouds(float deltaTime);
    float m_jobSiteTimer = 0.0f;
    static constexpr int JOB_SITE_REACH = 4;
    void setBlockFor(const glm::ivec3& at, BlockId now, BlockId then, float seconds);
    void tickTimedBlocks(float deltaTime);
    uint32_t m_wearSeed = 0x2545F491u;
    bool m_particleDemo = false;
    void tickSpriteParticles(float deltaTime);
    void prepareMobModel();
    void sprites(SpriteParticles::Kind kind, const glm::vec3& at, int count, float spread, float speed,
                 float size, float lifetime, float gravity = 0.0f);
    // One texture per character in the picker. Painting eight 64x32
    // skins costs well under a millisecond, so the whole library is kept
    // resident rather than rebuilt as you scroll through it.
    std::vector<std::unique_ptr<PlayerSkin>> m_skins;
    PlayerSkin m_customSkin;
    int m_skinVariant = 0;
    int m_pendingSkinVariant = 0;   // which swatch was clicked this frame
    PlayerSkin::Style m_skinStyle = PlayerSkin::Style::Classic;

    // Multiplayer. Idle until the world is opened or one is joined, so
    // singleplayer costs nothing but a few bytes of state.
    Net::Session m_net;
#ifndef __EMSCRIPTEN__
    Net::LanDiscovery m_discovery;
#endif
    // Drawing everybody else. Only the choice of character crosses the
    // wire, so each one someone is wearing gets painted here on demand
    // and kept; the key is the variant and the model type together.
    PlayerModel m_playerModel;

    // The figure on the inventory screen, built from the same model as
    // everyone in the world and flattened onto the interface. Kept apart
    // from the one above so drawing the panel never disturbs a buffer the
    // world pass is using.
    PlayerModel m_dollModel;
    EntityManager m_entities;
    MobModel m_mobModel;
    std::map<int, std::unique_ptr<PlayerSkin>> m_remoteSkins;

    // Limb state cannot come off the wire -- positions arrive twenty
    // times a second and the walk cycle runs every frame -- so it is
    // worked out here from how far each player has moved. Keyed by
    // player id, with the local player under LOCAL_ANIM.
    struct Animation
    {
        float phase = 0.0f;     // radians around the walk cycle
        float amount = 0.0f;    // 0 standing, 1 at walking pace
        float bodyYaw = 0.0f;   // trails the head
        glm::vec3 lastPosition{ 0.0f };
        bool started = false;
    };
    static constexpr uint32_t LOCAL_ANIM = 0xFFFFFFFFu;
    std::map<uint32_t, Animation> m_animation;

    // Name tags, gathered while the world is drawn and written out in
    // the HUD pass: the text has to go on top of everything, but only
    // the world pass knows where on screen each player ended up.
    struct Nameplate
    {
        std::string name;
        float x = 0.0f, y = 0.0f;
        float scale = 1.0f;
        bool sign = false;
        bool glow = false;
        glm::vec3 fill{ 1.0f };
        glm::vec3 edge{ 0.0f };
    };
    std::vector<Nameplate> m_nameplates;

    // Seconds of flight a pair of wings has done since the last point
    // of wear was taken off them.
    float m_elytraWear = 0.0f;

    Signs m_signs;
    Frames m_frames;
    bool m_signOpen = false;
    glm::ivec3 m_signAt{ 0 };
    Signs::Text m_signDraft;
    int m_signLine = 0;

    std::vector<BookText> m_bookTexts;
    std::map<std::array<int, 4>, ItemStack> m_lecternBooks;
    bool m_bookOpen = false;
    bool m_bookEditing = false;
    bool m_bookSigning = false;
    bool m_bookFromLectern = false;
    int m_bookPage = 0;
    int m_bookSlot = 0;
    glm::ivec3 m_bookLectern{ 0 };
    BookText m_bookShown;
    std::vector<std::string> m_bookDraft;
    std::string m_bookTitle;

    // Minecraft's F5: your own eyes, behind you, or facing you.
    enum class Perspective { FirstPerson, ThirdBack, ThirdFront };
    Perspective m_perspective = Perspective::FirstPerson;

    std::string m_addressDraft = "127.0.0.1";
    // A guest may already be standing in a singleplayer world when it
    // joins, so "has the world been rebuilt from the host's seed yet?"
    // cannot be answered by looking at m_world.
    bool m_joinedWorldBuilt = false;
    int m_joinChoice = -1;      // index into the discovered list, set by a click

    Screen m_screen = Screen::Title;
    MenuAction m_pendingAction = MenuAction::None;
    GameMode m_gameMode = GameMode::Survival;
    bool m_hardcoreDeath = false; // the run is over; no respawn offered

    // Footstep pacing and landing detection.
    float m_distanceWalked = 0.0f;
    bool m_wasOnGround = true;

    // The bench the 3x3 grid belongs to, and whether the open inventory
    // is showing it. Walking away from it shuts the grid, as Minecraft
    // does, so the big grid cannot be carried around.
    glm::ivec3 m_benchBlock{ 0 };
    bool m_benchOpen = false;

    // Where each lit stick of TNT is and how long it has left. Small
    // enough and short-lived enough not to be worth saving: a world
    // reloaded mid-blast simply has the TNT still standing.
    std::vector<std::pair<glm::ivec3, float>> m_fuses;

    // How long the player has been standing in a portal, and whether a
    // way back needs building where they come out.
    float m_portalTimer = 0.0f;
    bool m_owedReturnPortal = false;

    // Where the overworld portal stood, so coming back puts you at the
    // one you left through rather than digging a new hole at spawn.
    glm::vec3 m_portalHome{ 0.0f };
    bool m_hasPortalHome = false;

    Projectiles m_arrows;
    float m_bowDraw = 0.0f;      // seconds the bow has been held
    float m_crossbowCharge = 0.0f;
    bool m_crossbowHeldOver = false;
    void updateCrossbow(float deltaTime);
    struct ThrownTrident
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        ItemStack stack;
        bool stuck = false;
        bool returning = false;
        bool struck = false;
        float age = 0.0f;
        int slot = 0;
    };
    std::vector<ThrownTrident> m_tridents;
    float m_tridentCharge = 0.0f;
    float m_riptideSpin = 0.0f;
    void updateTrident(float deltaTime);
    void tickTridents(float deltaTime);
    void boltAt(const glm::vec3& strike);
    bool inWaterOrRain() const;
    int crossbowIcon(const ItemStack& stack, bool selected) const;
    int ammoSlot() const;
    bool m_bowDrawing = false;
    int m_iconOverride = -1;
    bool m_arrowDemo = false;   // dev aid: keep arrows in the air for a screenshot

    Chests m_chests;
    glm::ivec3 m_chestBlock{ 0 };
    bool m_chestOpen = false;
    int64_t m_chestVehicle = -1;
    BlockId m_chestVehicleBlock = Blocks::Chest;
    std::string m_chestVehicleName;
    std::map<uint32_t, Chests::Contents> m_vehicleChests;
    std::map<uint32_t, float> m_cartFuel;
    std::map<uint32_t, glm::vec2> m_cartPush;
    std::map<uint32_t, float> m_cartFuses;
    std::map<uint32_t, std::string> m_cartCommands;
    int64_t m_editorCart = -1;
    void primeTntCart(Mob& cart);
    struct FireworkFlight
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        float age = 0.0f;
        float life = 1.0f;
        float tick = 0.0f;
        int design = 0;
        bool shot = false;
        bool boost = false;
    };
    struct FireworkSpark
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        unsigned int colour = 0xFFFFFFu;
        unsigned int fade = 0xFFFFFFu;
        bool fades = false;
        bool trail = false;
        bool twinkle = false;
        float age = 0.0f;
        float life = 2.5f;
        float tick = 0.0f;
        float size = 0.06f;
    };
    std::vector<FireworkFlight> m_fireworks;
    std::vector<FireworkSpark> m_sparks;
    std::vector<std::pair<glm::vec3, float>> m_twinkles;
    FireworkRenderer m_fireworkRenderer;
    void launchFirework(int design, const glm::vec3& at, const glm::vec3& velocity, bool shot, bool boost);
    void burstFirework(const glm::vec3& at, int design);
    void tickFireworks(float deltaTime);
    struct PotionCloud
    {
        glm::vec3 position{ 0.0f };
        int effect = 0;
        float age = 0.0f;
        float radius = 3.0f;
        float shrunk = 0.0f;
        float tick = 0.0f;
        float puff = 0.0f;
        float playerTouched = -10.0f;
        uint32_t seed = 0x2545F491u;
        std::map<uint32_t, float> touched;
    };
    std::vector<PotionCloud> m_potionClouds;
    void splashPotion(const glm::vec3& at, int effect);
    void effectDeath(const glm::vec3& at, float height, bool oozing, bool weaving, bool windCharged);
    void windBurst(const glm::vec3& at);
    void spawnSilverfish(const glm::vec3& at, int count);
    void tickPotionClouds(float deltaTime);
    void tickArmadillos(float deltaTime);
    void conjureFangs(const glm::vec3& caster, const glm::vec3& target, int damage);
    void tickFangs(float deltaTime);
    void tickGlowSquids(float deltaTime);
    bool placePainting(ItemStack& stack);
    bool hitPainting(float blockDistance);
    void tickPaintings(float deltaTime);
    void dropPainting(const Paintings::Placed& painting, bool drop);
    bool placeArmorStand(ItemStack& stack);
    bool useArmorStand();
    bool hitArmorStand(float blockDistance);
    void breakArmorStand(size_t index, bool drop);
    void tickArmorStands(float deltaTime);
    void pushArmorStands(const glm::ivec3& from, int facing, int length);
    std::vector<ArmorStands::Stand> standsHere(float reach) const;
    SignRenderer m_signRenderer;
    ArmourTextures m_armourTextures;
    void tickSigns(float deltaTime);
    void settleExplorerMaps();
    void drawMapMarks(const Maps::Data& map, int id, float x, float y, float size, bool withPlayer);
    void renderFramedMaps(Shader& shader);
    std::map<int, unsigned int> m_framedMapTextures;
    float m_framedMapClock = 0.0f;
    Mesh m_framedMapMesh;
    float m_signClock = 0.0f;
    ArmorStandStore m_stands;
    ArmorStandRenderer m_standRenderer;
    PaintingStore m_paintings;
    PaintingRenderer m_paintingRenderer;
    float m_paintingClock = 0.0f;
    std::vector<Fangs::Fang> m_fangs;
    FangRenderer m_fangRenderer;
    float m_fangClock = 0.0f;
    float m_glowClock = 0.0f;
    void queueFireworks();
    struct ParkedVehicle
    {
        int dimension = 0;
        MobId type = MobId::Boat;
        int variant = 0;
        glm::vec3 position{ 0.0f };
        float yaw = 0.0f;
        float fuel = 0.0f;
        std::string command;
        bool hasContents = false;
        Chests::Contents contents{};
    };
    std::vector<ParkedVehicle> m_parkedVehicles;
    std::map<std::array<int, 4>, int> m_cageKinds;
    std::map<std::array<int, 4>, ItemStack> m_jukeboxes;
    std::map<std::array<int, 4>, float> m_jukeboxClock;
    std::array<int, 4> m_jukeboxPlaying{ -99, 0, 0, 0 };
    bool useJukebox(const glm::ivec3& at);
    void ejectJukebox(const glm::ivec3& at, bool drop);
    void tickJukeboxes(float deltaTime);
    float m_vehicleClock = 0.0f;
    ParkedVehicle parkOf(const Mob& vehicle) const;
    void parkVehicles(bool onlyFar);
    void unparkVehicles();
    void tickCarts(float deltaTime);
    void openVehicleChest(Mob& vehicle);
    bool m_chestDouble = false;
    bool m_chestEnder = false;
    glm::ivec3 m_chestPartner{ 0 };

    // Three by three, which is the whole of what a crafter is.
    static constexpr int CRAFTER_SLOTS = 9;
    static constexpr int CRAFTER_MASK_SLOT = 26;
    bool crafterSlotDisabled(int cell);
    void setCrafterSlotDisabled(int cell, bool disabled);

    // A crafter keeps what is in it in the same table a chest does: it
    // is a nine-slot chest with a trigger, and giving it a table of its
    // own would mean a second thing to save, load and forget about.
    glm::ivec3 m_crafterBlock{ 0 };
    bool m_crafterOpen = false;

    Furnaces m_furnaces;
    glm::ivec3 m_furnaceBlock{ 0 };
    bool m_furnaceOpen = false;

    // The open enchanting table. The shelf count and the roll are taken
    // once when it is opened and held until it shuts, so the three
    // offers stand still while you look at them rather than flickering
    // every frame -- and so walking a shelf into place while the screen
    // is open does not change the price under your hand.
    // The villager being traded with, held by the seed it was born
    // with rather than by a pointer: mobs live in a vector that moves
    // when it grows, and one can wander off or be killed while its
    // screen is open.
    // Where you stood when you stepped into the end portal, so
    // stepping back out of one in the End puts you back there rather
    // than at the world spawn.
    glm::vec3 m_endHome{ 0.0f };
    bool m_hasEndHome = false;

    bool m_tradeOpen = false;
    uint32_t m_tradeSeed = 0;
    Trading::Profession m_tradeWith = Trading::Profession::Farmer;

    // What each villager you have traded with remembers: how far it has
    // come, how much of each deal it has left, and when it restocks.
    // Kept here and saved with the world, because mobs are not: a
    // villager that has wandered out of range is brought back where it
    // was when you return.
    struct TraderRecord
    {
        uint32_t seed = 0;
        int profession = 0;
        int dimension = 0;
        glm::vec3 position{ 0.0f };
        int xp = 0;
        std::vector<int> uses;
        float restockIn = Trading::RESTOCK_SECONDS;
    };
    std::unordered_map<uint32_t, TraderRecord> m_traders;
    int m_tradeSelected = -1;
    int m_tradeScroll = 0;
    float m_traderSyncTimer = 0.0f;
    TraderRecord& openTrader();
    void selectTrade(int deal);
    void refreshTradeResult();
    void takeTradeResult(bool all);
    void stowTrading();
    void updateTraders(float deltaTime);
    void saveTraders();
    void loadTraders();

    bool m_enchantOpen = false;
    float m_drinkTimer = 0.0f;
    bool m_brewOpen = false;
    glm::ivec3 m_brewBlock{ 0 };
    float m_brewProgress = 0.0f;
    int m_brewFuel = 0;          // brews left in the powder that is burning
    bool m_anvilOpen = false;
    glm::ivec3 m_anvilBlock{ 0 };
    glm::ivec3 m_enchantBlock{ 0 };
    int m_enchantShelves = 0;
    uint32_t m_enchantRoll = 0;
    bool m_pendingFurnaceLight = false;

    // How far down the creative palette has been scrolled.
    int m_paletteRow = 0;

    // Set while the creative scrollbar's thumb is being dragged, which
    // lasts from the press on the track until the button comes up.
    bool m_draggingScroll = false;
    int m_previousHealth = Player::MAX_HEALTH;

    std::string m_savePath = "world";
    glm::vec3 m_spawnPoint{ 0.0f };
    bool m_hasAnchor = false;
    bool m_anchorPending = false;
    glm::ivec3 m_anchor{ 0 };
    float m_magmaSting = 0.0f;
    std::map<int, Maps::Data> m_maps;
    std::set<int> m_dirtyMaps;
    int m_nextMapId = 1;
    float m_mapClock = 0.0f;
    int m_mapRow = 0;
    unsigned int m_mapTexture = 0;
    int m_mapTextureId = -1;
    bool m_mapTextureStale = true;
    Maps::Data* mapData(int id);
    void tickMaps(float deltaTime);
    void renderHeldMap(float w, float h);
    unsigned int mapColourAt(int x, int z, int& height);
    bool m_cartoOpen = false;
    glm::ivec3 m_cartoBlock{ 0 };
    void openCartography(const glm::ivec3& block);
    void stowCartography();
    void takeCartoResult(bool toInventory);
    ItemStack cartoResult();
    Banners::Library m_bannerLibrary;
    std::map<std::array<int, 4>, int> m_bannerAt;
    std::vector<BannerRenderer::Placed> nearbyBanners();
    void drawBannerFlag(int colour, const Banners::Design& design, float x, float y, float w, float h, const glm::vec4& tint);
    bool m_loomOpen = false;
    glm::ivec3 m_loomBlock{ 0 };
    int m_loomChoice = -1;
    int m_loomScroll = 0;
    StackId m_loomSeen = Blocks::Air;
    void openLoom(const glm::ivec3& block);
    void stowLoom();
    void takeLoomResult(bool toInventory);
    bool loomPreview(Banners::Design& design);
    void drawBannerIcon(float x, float y, float size, int colour, const Banners::Design& design, const glm::vec4& tint);
    unsigned int mapTextureFor(int id);
    std::map<std::array<int, 4>, uint32_t> m_potSides;
    void pinPotPatterns();

    // 0 = midnight, 0.5 = noon. Mid-morning: bright enough to show the
    // scenery off, with the sun still low enough to be interesting.
    static constexpr float TITLE_TIME_OF_DAY = 0.38f;
    float m_timeOfDay = TITLE_TIME_OF_DAY;
    int m_dayNumber = 0;
    float m_elapsedSeconds = 0.0f;
    float m_lastFrameSeconds = 0.016f;
    float m_timeOverride = -1.0f;
    float m_autosaveTimer = 0.0f;
    float m_titleSpin = 0.0f;            // slow camera drift behind the menus
    bool m_menuWorld = false;            // the loaded world is the title panorama
    glm::vec3 m_menuFocus{ 0.0f };       // point the title camera orbits

    std::string m_playerName;   // empty until the first visit picks one
    std::string m_nameDraft;    // what is being typed on the name screen

    bool m_inventoryOpen = false;
    bool m_showDebug = false;
    // How much the debug screen shows. F3+F6 cycles it, the way F3+F4
    // and friends change the screen in Minecraft.
    enum class DebugDetail { Minimal, Default, Full };
    DebugDetail m_debugDetail = DebugDetail::Default;
    bool m_hudHidden = false;   // F1
    bool m_running = true;

    // Physics stays frozen until the chunk under the player exists --
    // otherwise they fall through an empty world and get buried when
    // terrain finally streams in.
    bool m_worldReady = false;
    bool m_placeOnSurface = false;

    // LabPBR normal/specular shading from the resource pack (toggle with P).
    bool m_pbrEnabled = true;

    float m_autoScreenshotDelay = 0.0f;
    float m_readySeconds = 0.0f;
    bool m_pendingScreenshot = false;
    bool m_automated = false;
    bool m_startImmediately = false;
    GameMode m_startupMode = GameMode::Survival;
    std::string m_startupScreen;

    // A lineup screen holds its mobs still. The screen name itself is
    // cleared once the world is set up, so the decision is kept here:
    // reading the name at update time, which is what this used to do,
    // froze nothing at all and every mob walked into the camera.
    bool m_freezeMobs = false;

    // The biome screens cannot cut their clearing the moment they
    // arrive: the chunks around where they teleported to have not
    // streamed in yet, and setBlock into a chunk that does not exist
    // does nothing. So the clearing waits until the world says it is
    // ready, which is the same gate the screenshot itself waits on.
    enum class ArenaKind { BiomePocket, ChamberProps, CaveProps, WitherAltar };
    bool m_pendingArena = false;
    ArenaKind m_arenaKind = ArenaKind::BiomePocket;
    int m_arenaX = 0, m_arenaY = 0, m_arenaZ = 0;
    float m_arenaDelay = 0.0f;
    void buildBiomeArena(int standX, int standZ);
    void dressChamberRoom(int atX, int atY, int atZ);
    void lightCave(int atX, int atY, int atZ);
    void raiseWitherForShot(int atX, int atY, int atZ);
    uint32_t m_seedOverride = 0;
    bool m_hasSeedOverride = false;
    bool m_hostOnStart = false;
    std::string m_joinOnStart;
    bool m_buildTest = false;
    bool m_buildTestDone = false;
    bool m_mobTest = false;
    bool m_mobTestDone = false;
    // Seconds before a gateway will take you anywhere again, so
    // arriving beside one does not send you straight back.
    float m_gatewayCooldown = 0.0f;
    bool m_swimTest = false;
    bool m_swimTestDone = false;
    std::string m_startAt;
    bool m_farmTest = false;
    bool m_farmTestDone = false;
    bool m_shopTest = false;
    bool m_shopTestDone = false;
    Dimension m_startupDimension = Dimension::Overworld;
    uint32_t m_panoramaSeed = 0;
    glm::vec3 m_panoramaSpot{ 0.0f };
    bool m_hasPanoramaOverride = false;

    unsigned long long m_previousTicks = 0;

    // Mining state
    bool m_hasTarget = false;
    glm::ivec3 m_targetBlock{ 0 };
    float m_breakProgress = 0.0f;
    float m_digSoundTimer = 0.0f;
    float m_attackTicker = 10.0f;
    StackId m_attackItem = 0;
    int m_attackIndicator = 1;
    int m_distortion = 100;
    float m_nausea = 0.0f;
    float m_darknessFade = 0.0f;
    int m_heartShown = 20;
    int m_heartLast = 20;
    float m_heartCheck = 0.0f;
    float m_heartJumpEnd = 0.0f;
    float m_devAttackStrength = -1.0f;
    bool m_leftHanded = false;
    float attackStrength() const;
    float attackPeriod() const;
    float m_eatTimer = 0.0f;
    float m_bushSting = 0.0f;
    float m_hiveClock = 0.0f;
    HeldItem m_heldItem;

    // Set before a world is made and saved with it, the way Minecraft
    // does: chat always works, the commands only here.
    bool m_allowCheats = false;

    Chat::Log m_chatLog;
    bool m_chatOpen = false;
    std::string m_chatDraft;
    float m_growthTimer = 0.0f;
    uint32_t m_growthSeed = 0x1F123BB5u;

    // Frame statistics
    float m_fps = 0.0f;
    float m_fpsAccumulator = 0.0f;
    int m_framesThisSecond = 0;

    bool frame();

    void handleEvents();
    void waitForSpawnChunk();
    void updateGameplay(float deltaTime);
    void updateMovementAudio(const glm::vec3& positionBefore, bool jumped);
    void updateMining(float deltaTime);
    void handlePlacement();

    void render();
    void renderWorld();
    void renderMenuBackdrop(float deltaTime);
    void renderHud();
    void renderGameModeSwitcher(float w, float h);
    void setGameMode(GameMode mode);
    bool m_gameModeSwitcher = false;
    int m_switcherChoice = 0;
    bool m_debugBeforeF3 = false;
    GameMode m_previousGameMode = GameMode::Survival;
    bool m_hadPreviousGameMode = false;
    void renderTitleScreen();
    void renderSingleplayerScreen();
    void renderMultiplayerScreen();
    void renderSettingsScreen();
    void renderControlsScreen();
    void renderNameEntryScreen();
    // Everyone else, and yourself when the camera is not behind your own
    // eyes. Called from renderWorld, inside the opaque pass.
    void renderPlayers(const glm::mat4& view, const glm::mat4& projection);
    void renderNameplates();
    // Steps every mob and plays whatever noise they made.
    void updateMobs(float deltaTime);
    // Mobs and crops together, for both the played and automated paths.
    void updateWorldAround(float deltaTime);
    // Advances every player's walk cycle from the ground they covered.
    void updatePlayerAnimation(float deltaTime);
    void advanceAnimation(Animation& animation, const glm::vec3& position,
                          float headYaw, float deltaTime);
    // The painted character someone picked, built the first time it is
    // asked for.
    const PlayerSkin& skinFor(const Net::Appearance& look);
    Net::Appearance localAppearance() const;
    // Where the camera actually sits this frame. The eye camera stays
    // put so that mining, placing and the selection outline keep working
    // from the player's eyes whatever the view is set to.
    Camera viewCamera() const;
    glm::mat4 worldProjection(const Camera& camera) const;
    void renderEffectOverlays(float w, float h);
    void renderPauseMenu();
    void renderDeathScreen();
    void renderInventoryScreen();

    // The two halves of it: every container the game has, and the
    // creative screen with its tabs, which is a different shape.
    void renderContainerScreen(float w, float h);
    void renderCreativeScreen(float w, float h);

    // What goes over the top of either: the highlight on the slot under
    // the cursor, the tooltip naming what is in it, and whatever stack
    // the cursor is carrying.
    void finishInventoryScreen();

    // The player in the box from (x1, y1) to (x2, y2), `size` interface
    // pixels to the block, turning towards the cursor and leaning with it
    // -- 1.21's inventory figure, numbers and all.
    void drawPlayerFigure(const PlayerSkin& skin, float x1, float y1, float x2, float y2,
                          float size);
    void drawMobFigure(const Mob& mob, float x1, float y1, float x2, float y2, float size);

    // The grey arrow between a crafting grid and what it makes, filled
    // white from the left as far as `filled` -- a furnace's progress.
    void drawCraftArrow(float x, float y, float width, float height, float filled = 0.0f);

    // A furnace's flame, fourteen square, burnt down to what is left.
    void drawFurnaceFlame(float x, float y, float alight);

    // Which creative tab is the player's own things, which only the
    // inventory screen's own file knows the order of.
    static int creativeInventoryTab();

    // Minecraft's tooltip: a near-black box with a purple edge, white
    // text, twelve pixels right of the cursor and twelve up, pulled
    // back on screen if it would run off it.
    void drawTooltip(const std::string& text, float mouseX, float mouseY, bool plain = false);
    // A block drawn as a cube seen from the corner, the way Minecraft
    // draws one in a slot. Returns false for anything that is not a
    // block with six faces -- an item, a flower, a liquid -- which the
    // caller then draws flat as before.
    bool drawBlockIcon(float x, float y, float size, StackId id, const glm::vec4& tint);

    void drawItem(float x, float y, float size, StackId id, int count, const glm::vec4& tint,
                  int damage = 0, uint32_t enchantments = 0);
    unsigned int shieldFace(StackId id, uint32_t enchantments);
    unsigned int m_glintItem = 0;
    unsigned int m_underwaterTexture = 0;
    unsigned int m_glintEntity = 0;
    unsigned int glintTexture(bool entity);
    glm::vec2 glintShift() const;
    void drawPlayerDoll(const PlayerSkin& skin, float x, float y, float unit);
    // Rebuilds every painted character at the current model type.
    void rebuildSkinLibrary();
    const PlayerSkin& activeSkin() const;
    void handleInventoryClick(int mouseX, int mouseY, bool rightButton, bool shiftHeld);
    void returnCursorToWorld();

    // Shutting the inventory, from the key, from Escape, or by walking
    // away from the bench that opened it.
    void closeInventory();

    // Spends a point of the held tool's durability, and clears the slot
    // with a snap when there is none left.
    void wearTool(BlockId brokenBlock);

    // Totting up what is worn, and spending it when a blow lands.
    int armourPoints() const;

    // The strongest protection anywhere in the suit. Minecraft adds the
    // four pieces together; this takes the best of them, which keeps a
    // single enchanted helmet from being worth a whole suit and still
    // rewards enchanting the rest.
    uint32_t armourProtection() const;
    void wearArmour(int incoming);
    // Costs the held shield a point for stopping a blow, and breaks
    // it when it runs out.
    void wearShield(int points);

    // What is being ridden, and getting off it.
    Mob* riddenMob();
    void reshapeRailsAround(const glm::ivec3& at);

    // Re-runs the power rule around a cell and tells the network about
    // whatever it changed. Every place, break and switch goes through
    // this, which is the only way a lamp twelve blocks away knows.
    void settleRedstone(const glm::ivec3& at);
    struct Vibration
    {
        glm::ivec3 sensor{ 0 };
        float due = 0.0f;
        int frequency = 0;
        glm::vec3 from{ 0.0f };
    };
    float m_shriekCooldown = 0.0f;
    int m_warningLevel = 0;
    float m_warningDecay = 0.0f;
    float m_elderCurseClock = 0.0f;
    float m_elderCurseShow = 0.0f;
    float m_wardenDarkClock = 0.0f;
    float m_roseClock = 0.0f;
    glm::ivec3 m_lastShriekStep{ -99999 };
    void tickEffectSources(float deltaTime);
    void shriekNear(const glm::ivec3& sensor);
    void shriek(const glm::ivec3& at);
    bool insideAncientCity(const glm::vec3& at) const;
    void renderElderCurse(float w, float h);
    std::vector<Vibration> m_vibrations;
    std::vector<glm::ivec3> m_sensorSpots;
    std::map<std::array<int, 3>, float> m_sensorQuiet;
    float m_sensorScan = 0.0f;
    float m_mobVibration = 0.0f;
    float m_vibrationClock = 0.0f;
    void vibrate(const glm::vec3& at, int frequency);
    void tickVibrations(float deltaTime);
    bool vibrationBlocked(const glm::vec3& from, const glm::vec3& to) const;
    bool calibratedHears(const glm::ivec3& at, int frequency);

    // Trial spawners with a wave out. One entry while a spawner is
    // fighting you and gone again when it pays up, so a chamber full
    // of them costs nothing until somebody walks in.
    struct ActiveTrial
    {
        glm::ivec3 at{ 0, 0, 0 };
        float since = 0.0f;
        // Decided when the spawner woke, not when it pays out: the
        // omen that made it ominous will have run out by then.
        bool ominous = false;
    };
    std::vector<ActiveTrial> m_trials;
    float m_trialScan = 0.0f;

    void tickTrialSpawners(float deltaTime);

    // The plain cages, which keep sending the same thing out for as
    // long as you stand near them.
    void tickMonsterCages(float deltaTime);
    float m_cageTimer = 0.0f;

    // The two that belong to a structure rather than to the dark. A
    // monument keeps guardians and an ancient city keeps a warden;
    // neither spawns the way everything else does, because neither is
    // allowed anywhere else.
    float m_lairTimer = 0.0f;
    void tickStructureLairs(float deltaTime);
    void tryRaiseGolem(const glm::ivec3& pumpkin);
    void stepThroughGateway();

    // The End's fight: the dragon, and the crystals that keep it up.
    // Kept apart from the lairs because it is not a spawn rule -- it
    // is one fight, with one of everything in it.
    void tickDragonFight(float deltaTime);

    // The beacons near the player, and what they are worth. Swept
    // rather than remembered: a beacon is a block, and a block has
    // nowhere to keep a list.
    void tickBeacons(float deltaTime);
    void tickConduits(float deltaTime);
    float m_beaconTimer = 0.0f;

    // Where the beacons are. Kept rather than swept: looking for them
    // meant a quarter of a million block lookups a second, which is
    // not affordable on the machine this has to run on. Nothing
    // generates a beacon -- the only way one exists is that somebody
    // placed it -- so watching place and break is exact, not a
    // guess.
    std::set<std::array<int, 3>> m_beacons;
    std::set<std::array<int, 3>> m_conduits;
    float m_conduitTimer = 0.0f;
    float m_conduitSweepTimer = 0.0f;
    int m_conduitSweepLayer = 0;
    float m_eggStep = 0.0f;
    float m_freezeSting = 0.0f;
    float m_frostClock = 0.0f;
    std::map<std::array<int, 3>, float> m_frostedIce;
    void tickFrost(float deltaTime);
    void tickFire(float deltaTime);
    bool lightFire(const glm::ivec3& at);
    std::set<std::array<int, 3>> m_fires;
    float m_fireClock = 0.0f;
    void tickWiring(float deltaTime);
    void scheduleDiode(const glm::ivec3& at, BlockId now, bool wantLit);
    bool comparatorWants(const glm::ivec3& at);
    bool containerSignal(const glm::ivec3& at);
    void rewireTripwire(const glm::ivec3& at);
    std::set<std::array<int, 3>> m_comparators;
    std::set<std::array<int, 3>> m_daylightDetectors;
    float m_wiringClock = 0.0f;
    float m_daylightClock = 0.0f;
    std::map<std::array<int, 4>, std::array<int, 2>> m_beaconPowers;
    bool m_beaconOpen = false;
    bool m_grindOpen = false;
    bool m_cutOpen = false;
    bool m_smithOpen = false;
    glm::ivec3 m_smithBlock{ 0 };
    void openSmithing(const glm::ivec3& block);
    void stowSmithing();
    void takeSmithResult(bool toInventory);
    glm::ivec3 m_cutBlock{ 0 };
    int m_cutChoice = -1;
    int m_cutScroll = 0;
    StackId m_cutSeen = 0;
    void openStonecutter(const glm::ivec3& block);
    void stowStonecutter();
    void takeCutResult(bool toInventory);
    glm::ivec3 m_grindBlock{ 0 };
    uint32_t m_grindRoll = 0x9E3779B9u;
    void openGrindstone(const glm::ivec3& block);
    void stowGrindstone();
    void takeGrindResult(bool toInventory);
    Grindstone::Result grindResult() const;
    glm::ivec3 m_beaconBlock{ 0 };
    int m_beaconPrimary = -1;
    int m_beaconSecondary = -1;
    void openBeacon(const glm::ivec3& block);
    void stowBeacon();
    void confirmBeacon();
    int beaconTiers() const;

    // Which note blocks are already sounding. A note block strikes on
    // the edge of a signal rather than while one is held: nothing can
    // store that in the block, so it is kept here.
    std::set<std::array<int, 3>> m_noteLive;

    // Observers that are mid-pulse, and how long each has left. An
    // observer sends a flash rather than a level, so something has to
    // turn it off again -- and a block has nowhere to count down in.
    std::map<std::array<int, 3>, float> m_observerPulse;
    void wakeObservers(const glm::ivec3& around);
    void tickObservers(float deltaTime);

    // The hoppers near the player, moving one item each. Swept over
    // the chest table rather than the world: a hopper with nothing in
    // it and nothing above it has nothing to do, and the table is the
    // only place a hopper with contents can be hiding.
    void tickHoppers(float deltaTime);
    float m_hopperTimer = 0.0f;
    std::set<std::array<int, 3>> m_hoppers;

    // Droppers and dispensers already answering a signal. Edge
    // triggered like the note block, and for the same reason: a held
    // lever should throw one thing, not a stream of them.
    std::set<std::array<int, 3>> m_throwerLive;
    void fireThrower(const glm::ivec3& at, bool dispenser);
    void openExitPortal();
    void tryRaiseWither(const glm::ivec3& skull);
    float m_dragonTimer = 0.0f;
    bool m_dragonSlain = false;

    // Whether one has been seen alive on this island. "There was one
    // and now there is not" is how its death is noticed: a mob that
    // has died is gone from the list, and nothing else reports it.
    bool m_dragonSeen = false;

    // Which copper bulbs are sitting in a live signal. A bulb flips on
    // the edge of a pulse rather than following the power, so it has to
    // remember whether it has already answered this one; the block id
    // is already spent saying whether it is lit.
    std::set<std::array<int, 3>> m_bulbLive;

    // Copper goes green on its own. Nothing else in the world does, so
    // this is the only clock the blocks themselves keep.
    void tickCopper(float deltaTime);
    float m_copperTimer = 0.0f;
    int m_copperTick = 0;
    void openVault(const glm::ivec3& block);

    // How long a button stays down, which is Minecraft's second.
    static constexpr float BUTTON_SECONDS = 1.0f;

    // Buttons let go on their own. One at a time is enough: nothing in
    // this game can press two.
    glm::ivec3 m_buttonAt{ 0, 0, 0 };
    float m_buttonTimer = 0.0f;
    void dismount();
    void wearOnSwing(StackId weapon);
    void openBench(const glm::ivec3& block);

    // The open furnace borrows three inventory slots while its screen is
    // up; these move its contents in and back out again.
    // The bow: drawn while Use is held, loosed when it is let go.
    void updateBow(float deltaTime);
    void updateArrows(float deltaTime);
    void looseArrow(float drawn);

    // A creeper going off: what it breaks and what it hurts.
    void applyBlast(const glm::vec3& at, float radius, int damage);

    // TNT that has been lit, and the fuses still burning.
    void primeTnt(const glm::ivec3& block);
    void updateFuses(float deltaTime);

    void swingDoor(const glm::ivec3& block);
    void sleepInBed(const glm::ivec3& block);
    void growSapling(const glm::ivec3& at, uint32_t seed);

    // Lighting a portal, standing in one, and building the one that
    // brings you back.
    bool lightPortal(const glm::ivec3& at);
    void updatePortal(float deltaTime);
    void buildReturnPortal();
    void openChest(const glm::ivec3& block);
    std::vector<VillageLoot::Stack> villageChestLoot(const glm::ivec3& block) const;
    void stowChest();
    void setTrappedOpen(const glm::ivec3& block, bool open);
    void openCrafter(const glm::ivec3& block);
    void stowCrafter();

    // What a crafter does when something tells it to: makes one of
    // whatever its grid spells and puts it out on top.
    bool fireCrafter(const glm::ivec3& block);
    // Fills a chest standing in a village building the first time it
    // is opened. Does nothing to one the player built.
    void fillVillageChest(const glm::ivec3& block);

    void openFurnace(const glm::ivec3& block);

    // The workstations that need no screen: each is one action on what
    // you are holding. True when it took it, so the caller can fall
    // through to placing a block when it did not.
    // A sign is hung, then written on. The editor is the chat line's
    // machinery over four short lines rather than one long one.
    void openSign(const glm::ivec3& at);
    bool useOnSign(const glm::ivec3& at);
    const BookText* bookOf(const ItemStack& stack) const;
    std::string itemLabel(const ItemStack& stack) const;
    void openBook(const ItemStack& stack, bool fromLectern, const glm::ivec3& lectern);
    void closeBook(bool keep);
    void saveBookDraft(bool sign);
    void updateBookInput();
    void renderBookScreen();
    ItemStack* lecternBookAt(const glm::ivec3& at);
    void closeSign();
    void updateSignInput();
    void renderSignEditor();
    void collectSignPlates(const glm::mat4& viewProjection,
                           float screenWidth, float screenHeight);

    // An item frame takes one thing and gives it back.
    bool useFrame(const glm::ivec3& at, BlockId block);
    void pinFramedItems();

    bool useCake(const glm::ivec3& at, BlockId block);
    bool useCandle(const glm::ivec3& at, BlockId block);
    bool useCandleCake(const glm::ivec3& at, BlockId block);
    bool plantCocoa(const glm::ivec3& at);
    bool useComposter(const glm::ivec3& at, BlockId block);
    bool useCauldron(const glm::ivec3& at, BlockId block);
    bool useLectern(const glm::ivec3& at, BlockId block);
    bool useGrindstone(const glm::ivec3& at);

    // Rain raises a cauldron that has the sky over it.
    void fillCauldronsInRain();
    // Looks for a completed ring of eyed frames round a frame that was
    // just filled, and opens the portal inside it if there is one.
    void lightEndPortalAround(const glm::ivec3& frame);

    void openTrading(const Mob& villager);

    const Mob* tradingVillager() const;

    void openEnchantingTable(const glm::ivec3& block);
    void stowEnchantingTable();

    void openBrewingStand(const glm::ivec3& block);
    void stowBrewingStand();
    // Runs only while the screen is open. A stand with nobody watching
    // it keeps nothing, which is the one way this differs from
    // Minecraft's -- the stand has nowhere to store a half-done brew
    // until it is a saved container in its own right.
    void tickBrewing(float deltaTime);

    void openAnvil(const glm::ivec3& block);
    void stowAnvil();
    // Works out what the two inputs would make and puts it in the
    // result slot. Cheap, and run after any click, for the same reason
    // the craft grid is: deciding which clicks could matter is harder
    // than doing it every time.
    // What the two inputs currently make, for the screen and for the
    // click that takes it. Const, so drawing cannot change anything.
    Anvil::Result anvilResult() const;
    void refreshAnvilResult();
    void takeAnvilResult();

    // Copies whatever an open container is holding back to where it
    // belongs, without shutting it. Saving used to call the stow
    // functions for this, which also close the thing -- so an autosave
    // while you had a chest open put its contents away and left you
    // staring at your own pockets.
    void syncOpenContainers();
    void buyEnchantment(int row);
    void stowFurnace();
    void tickFurnaces(float deltaTime);
    void setFurnaceLit(const glm::ivec3& block, bool lit);

    void renderDebugOverlay();
    void renderHand();
    void renderChat();
    void updateChatInput();
    void runChatLine(const std::string& line);
    bool runCommand(const std::string& line, const glm::vec3& origin, bool fromBlock, std::string& output);
    struct CommandState
    {
        std::string command;
        std::string output;
        bool alwaysActive = false;
        bool powered = false;
        bool success = false;
    };
    struct StructureState
    {
        std::string name = "structure";
        glm::ivec3 offset{ 0, 1, 0 };
        glm::ivec3 size{ 5, 5, 5 };
    };
    std::map<std::array<int, 4>, CommandState> m_commands;
    std::map<std::array<int, 4>, StructureState> m_structures;
    float m_commandClock = 0.0f;
    void tickCommandBlocks(float deltaTime);
    void executeCommandBlock(const glm::ivec3& at, int depth);
    std::array<int, 4> blockKey(const glm::ivec3& at) const
    {
        return { static_cast<int>(m_dimension), at.x, at.y, at.z };
    }
    bool m_editorOpen = false;
    bool m_editorStructure = false;
    glm::ivec3 m_editorAt{ 0 };
    std::array<std::string, 3> m_editorFields;
    int m_editorFocus = 0;
    int m_editorKind = 0;
    bool m_editorConditional = false;
    bool m_editorAlways = false;
    int m_editorMode = 0;
    std::string m_editorStatus;
    void openBlockEditor(const glm::ivec3& at);
    void closeBlockEditor(bool keep);
    void updateBlockEditor();
    void renderBlockEditor();
    bool saveStructure(const glm::ivec3& at, const StructureState& state, std::string& message);
    bool loadStructure(const glm::ivec3& at, const StructureState& state, std::string& message);
    std::vector<glm::ivec3> m_markerSpots;
    float m_markerScan = 0.0f;
    void pinCreativeMarkers();
    struct Bobber
    {
        bool active = false;
        glm::vec3 position{ 0.0f };
        glm::vec3 velocity{ 0.0f };
        bool inWater = false;
        bool stuck = false;
        float wait = -1.0f;
        float approach = 0.0f;
        float bite = 0.0f;
        float age = 0.0f;
        uint32_t hooked = 0;
        uint32_t rolls = 0;
    };
    Bobber m_bobber;
    void castBobber();
    void reelBobber();
    void tickFishing(float deltaTime);
    glm::vec3 rodTip() const;
    bool holdingRod() const;
    void wearHeld(StackId id, int points);
    std::vector<std::string> m_itemNames;
    std::vector<std::array<int, 4>> m_lodestones;
    bool m_hasDeathPoint = false;
    glm::vec3 m_deathPoint{ 0.0f };
    int m_deathDimension = 0;
    bool m_scoping = false;
    bool m_devScope = false;
    int m_devHoverHotbar = -1;
    float m_scopeAmount = 0.0f;
    int animatedIcon(const ItemStack& stack) const;
    std::vector<std::vector<ItemStack>> m_bundles;
    float m_hornCooldown = 0.0f;
    static constexpr int BUNDLE_CAPACITY = 64;
    int itemWeight(const ItemStack& stack) const;
    int bundleWeight(const ItemStack& bundle) const;
    const std::vector<ItemStack>* bundleContents(const ItemStack& bundle) const;
    int bundleInsert(ItemStack& bundle, const ItemStack& adding);
    ItemStack bundleTakeLast(ItemStack& bundle);
    bool bundleClick(int slot);
    void drawBundleTooltip(const ItemStack& bundle, float mouseX, float mouseY);
    int nameIdOf(const std::string& name);
    std::string m_anvilName;
    bool m_anvilNameEdited = false;
    struct Leash
    {
        uint32_t mob = 0;
        bool toKnot = false;
        glm::ivec3 knot{ 0 };
    };
    std::vector<Leash> m_leashes;
    Mob* mobBySerial(uint32_t serial);
    Advancements::Tracker m_advancements;
    bool m_advOpen = false;
    int m_advTab = -1;
    float m_advOriginX[Advancements::TAB_COUNT] = {};
    float m_advOriginY[Advancements::TAB_COUNT] = {};
    bool m_advPlaced[Advancements::TAB_COUNT] = {};
    float m_advHoverFade = 0.0f;
    bool m_advDragging = false;
    float m_advDragX = 0.0f;
    float m_advDragY = 0.0f;
    float m_advCheck = 0.0f;
    bool m_levitating = false;
    int m_devAdvHover = -1;
    float m_levitateFrom = 0.0f;
    struct Toast
    {
        int advancement = -1;
        std::vector<StackId> recipes;
        float age = 0.0f;
        bool sounded = false;
        std::string systemTitle;
        int tutorialIcon = -1;
        std::string tutorialTitle;
        std::string tutorialDescription;
        bool progressBar = false;
        float progress = 0.0f;
        float shownProgress = 0.0f;
        bool hidden = false;
        float hiddenAt = 0.0f;
        uint32_t serial = 0;
    };
    std::vector<Toast> m_toasts;
    enum class TutorialStep : uint8_t { Movement, FindTree, PunchTree, OpenInventory, CraftPlanks, None };
    TutorialStep m_tutorialStep = TutorialStep::Movement;
    int m_tutorialTicks = 0;
    int m_tutorialMoved = 0;
    int m_tutorialLooked = 0;
    int m_tutorialMoveDone = -1;
    int m_tutorialLookDone = -1;
    int m_tutorialPunches = 0;
    bool m_tutorialMovedNow = false;
    bool m_tutorialLookedNow = false;
    bool m_tutorialForced = false;
    float m_tutorialClock = 0.0f;
    float m_tutorialYaw = 0.0f;
    float m_tutorialPitch = 0.0f;
    long long m_tutorialLogs = 0;
    uint32_t m_tutorialToast = 0;
    uint32_t m_tutorialLookToast = 0;
    uint32_t m_toastSerial = 0;
    static const char* tutorialStepId(TutorialStep step);
    static TutorialStep tutorialStepFrom(const std::string& id);
    void drawBoldText(const std::string& text, float x, float y, const glm::vec4& colour);
    uint32_t addTutorialToast(int icon, const std::string& title, const std::string& description, bool progressBar);
    Toast* toastBySerial(uint32_t serial);
    void hideTutorialToast(uint32_t& serial);
    void setTutorialStep(TutorialStep step);
    long long minedTreeBlocks(bool logsOnly) const;
    bool inventoryHas(const std::function<bool(StackId)>& test) const;
    void tickTutorial(float deltaTime);
    void drawTutorialToast(const Toast& toast, float tx, float ty);
    struct SpectatorCommand
    {
        enum class Kind : uint8_t { Blank, TeleportGroup, TeamGroup, Player, PreviousPage, NextPage, Close };
        Kind kind = Kind::Blank;
        std::string name;
        bool enabled = false;
        uint32_t player = 0;
    };
    bool m_spectatorMenu = false;
    int m_spectatorSlot = -1;
    int m_spectatorGroup = 0;
    int m_spectatorPage = 0;
    uint32_t m_spectatorTouched = 0;
    uint32_t m_spectatingMob = 0;
    std::vector<SpectatorCommand> spectatorGroup() const;
    SpectatorCommand spectatorCommand(int slot) const;
    void useSpectatorSlot(int slot);
    void openSpectatorMenu();
    void closeSpectatorMenu();
    float spectatorMenuShown() const;
    void updateSpectatorInput();
    bool followSpectatedMob();
    void renderSpectatorMenu(float w, float h);
    float m_textBackground = 0.5f;
    float m_chatOpacity = 1.0f;
    float m_fovEffects = 1.0f;
    float m_darknessPulsing = 1.0f;
    float m_panoramaSpeed = 1.0f;
    bool m_toggleSneak = false;
    bool m_toggleSprint = false;
    bool m_hideLightning = false;
    bool m_hideSplash = false;
    bool m_sneakLatched = false;
    bool m_sprintLatched = false;
    int m_activeSlider = -1;
    Screen m_accessReturn = Screen::Title;
    std::string m_titleTip;
    bool guiSlider(int id, float x, float y, float w, const std::string& label, float& value);
    void renderAccessibility();
    void renderLanguage();
    bool titleIconButton(float x, float y, float size, const char* icon, const char* tooltip);
    bool m_bindingCaptured = false;
    void setDifficulty(Worlds::Difficulty difficulty);
    bool lockButton(float x, float y, float size, bool locked, bool enabled);
    void renderLockConfirm();
    std::set<StackId> m_knownRecipes;
    bool m_recipesSeeded = false;
    StackId stackNamed(const std::string& id) const;
    std::string entityId(MobId id) const;
    void award(const char* id, const char* criterion);
    void awardAll(const char* id);
    bool inventoryHolds(const std::string& items) const;
    void tickAdvancements(float deltaTime);
    void unlockRecipes(float deltaTime);
    float m_recipeClock = 0.0f;
    void openAdvancements();
    void closeAdvancements();
    std::vector<int> advancementTabs() const;
    void updateAdvancementsInput();
    void renderAdvancementsScreen();
    void renderToasts();
    void saveAdvancements();
    void loadAdvancements(bool existing);
    Stats::Book m_stats;
    int m_statsTab = 0;
    float m_statsScroll = 0.0f;
    bool m_statsScrolling = false;
    float m_statsGrabY = 0.0f;
    int m_statsSelected = -1;
    int m_statsSort = -1;
    int m_statsOrder = 0;
    int m_statsPressed = -1;
    float m_statsClock = 0.0f;
    float m_statsFall = 0.0f;
    bool m_statsPlaced = false;
    glm::vec3 m_statsFrom{ 0.0f };
    int m_lastAttacker = -1;
    float m_lastAttackerAge = 0.0f;
    int m_blastSource = -1;
    void statBroke(StackId id);
    StackId statBlockItem(BlockId block) const;
    void statOpened(BlockId block);
    void statUsed(const ItemStack& before, const ItemStack& after);
    void drainStatEvents();
    void tickStats(float deltaTime);
    void openStatistics();
    std::vector<StackId> statItemRows() const;
    std::vector<int> statMobRows() const;
    void renderStatsScreen();
    void saveStats();
    void loadStats();
    std::string m_configPath = "world";
    Worlds::Meta m_worldMeta;
    std::vector<Worlds::Entry> m_worldEntries;
    struct GuiList
    {
        float scroll = 0.0f;
        bool dragging = false;
        float grabY = 0.0f;
        int selected = -1;
        uint32_t clickTime = 0;
        int clickIndex = -1;
        int clicked = -1;
        bool doubleClicked = false;
        int hovered = -1;
    };
    struct GuiField
    {
        std::string text;
        size_t maxLength = 32;
        bool focused = false;
        uint32_t focusTime = 0;
    };
    struct CreateWorldState
    {
        int tab = 0;
        int mode = 0;
        Worlds::Difficulty difficulty = Worlds::Difficulty::Normal;
        int cheats = -1;
        Worlds::Type type = Worlds::Type::Default;
        bool structures = true;
        bool bonusChest = false;
        std::string layers = "minecraft:bedrock,2*minecraft:dirt,minecraft:grass_block";
        std::string flatBiome = "minecraft:plains";
        std::string biome = "minecraft:plains";
        bool fromTitle = false;
    };
    CreateWorldState m_create;
    GuiList m_worldList, m_flatList, m_presetList, m_biomeList;
    GuiList m_accessList, m_languageList, m_controlsList;
    GuiField m_worldSearch, m_createName, m_createSeed, m_editName, m_presetField;
    std::string m_editFolder;
    std::string m_editNote;
    std::string m_flatDraft;
    std::string m_flatBiomeDraft;
    std::string m_biomeDraft;
    std::string m_pendingWorldFolder;
    bool m_iconWanted = false;
    bool m_bonusChestPending = false;
    bool m_seedChosen = false;
    uint32_t m_seedChoice = 0;
    std::string savesDirectory() const { return m_configPath + "/saves"; }
    void migrateLegacySave();
    void refreshWorlds();
    std::string defaultWorldFolder();
    void openWorldList();
    void playWorld(const std::string& folder);
    void openCreateWorld(const Worlds::Entry* from);
    void createWorldFromScreen();
    WorldGen::Options generatorOptions(const Worlds::Meta& meta) const;
    std::vector<std::pair<BlockId, int>> flatLayers(const std::string& text) const;
    bool createCheats() const;
    void placeBonusChest();
    void writeWorldMeta();
    bool guiButton(float x, float y, float w, float h, const std::string& label, bool enabled = true);
    bool guiCycle(float x, float y, float w, const std::string& label, bool enabled = true);
    void guiField(GuiField& field, float x, float y, float w, float h, const std::string& placeholder = std::string());
    void guiList(GuiList& list, float top, float height, float rowHeight, float rowWidth, int count, float header,
                 const std::function<void(int, float, float, bool)>& drawRow, float listX = 0.0f, float listW = 0.0f);
    void menuScreenBackdrop(float w, float h);
    void wrappedTooltip(const std::string& text, float mouseX, float mouseY, float width);
    void renderCreateWorld();
    void renderEditWorld();
    void renderConfirmDelete();
    void renderCustomizeFlat();
    void renderFlatPresets();
    void renderCustomizeBiome();
    std::vector<Servers::Entry> m_servers;
    std::vector<Net::Pinger> m_pings;
    GuiList m_serverList;
    GuiField m_serverName, m_serverAddress, m_directAddress;
    int m_editServer = -1;
    Servers::Packs m_serverPacks = Servers::Packs::Prompt;
    bool m_deletingServer = false;
    std::string m_lastDirect;
    void saveServers();
    void refreshPings();
    void openServerList();
    void joinAddress(const std::string& address);
    void renderEditServer();
    void renderDirectConnect();
    void renderConnecting();
    std::vector<std::string> m_packSelection;
    std::vector<Packs::Info> m_packsAvailable, m_packsSelected;
    GuiList m_availableList, m_selectedList;
    int m_packConfirm = -1;
    Screen m_packsReturn = Screen::Settings;
    bool m_reloadRequested = false;
    std::string m_reloadWorld;
    std::string m_resumeFolder;
    void openPacks();
    void applyPacks();
    void requestReload();
    void renderPacks();
public:
    bool reloadRequested() const { return m_reloadRequested; }
    const std::string& reloadWorld() const { return m_reloadWorld; }
    void setResumeWorld(const std::string& folder) { m_resumeFolder = folder; }
private:
    Raids::Raid m_raid;
    glm::vec3 m_raidOmenAt{ 0.0f };
    bool m_raidOmenPending = false;
    int m_raidOmenLevel = 1;
    float m_raidOmenEnds = 0.0f;
    float m_raidSteer = 0.0f;
    void tickRaid(float deltaTime);
    void startRaid(const glm::vec3& at, int omenLevel);
    void spawnRaidWave();
    int villagersNear(const glm::vec3& at, float radius) const;
    std::vector<Trading::Deal> tradeDeals() const;
    void tickLeashes(float deltaTime);
    void dropLeash(const Leash& leash, const glm::vec3& at);
    float renderHearts(float x, float y);
    void effectIcon(Effects::Kind kind, float x, float y, float size, float alpha);
    void renderArmour(float x, float y);
    void renderFood(float rightX, float y);
    void renderExperience(float x, float y, float width);
    void renderAir(float rightX, float y);
    void renderEffectIcons(float w);
    void drawInventoryEffects(float panelRight, float panelTop, float w);
    void renderEatProgress(float cx, float cy);
    void handleEating(float deltaTime);
    void handleBrushing(float deltaTime);
    float m_brushTime = 0.0f;
    glm::ivec3 m_brushAt{ 0 };
    void handleDrinking(float deltaTime);

    // --- simple immediate-mode menu widgets ---
    struct Rect { float x, y, w, h; };
    bool menuButton(const Rect& rect, const std::string& label, bool enabled = true);

    // Minecraft's whole interface is one shape: a flat slab with a
    // two-pixel bevel, light along the top and left and dark along the
    // bottom and right. Invert it and the same slab reads as a hole,
    // which is all an inventory slot is.
    // A filled rectangle with its corners stepped in. Minecraft cuts the
    // corner pixels off its panels and buttons rather than drawing a
    // true curve, which at interface scale reads as a soft corner and
    // stays crisp at any size -- a real radius would blur.
    void roundedQuad(float x, float y, float w, float h, const glm::vec4& colour, float cut = 4.0f);
    void bevel(float x, float y, float w, float h, const glm::vec4& face,
               const glm::vec4& light, const glm::vec4& dark, float edge = 2.0f);
    // How many screen pixels one interface pixel is drawn as, by
    // Minecraft's rule: the largest whole number that still leaves a
    // canvas of at least 320 by 240 to lay the interface out on.
    int guiScale() const;

    // 0 is automatic, which is Minecraft's default and takes the
    // largest scale the window will hold; 1 to 4 pin it.
    int m_guiScaleSetting = 0;

    void guiPanel(float x, float y, float w, float h);
    void guiWell(float x, float y, float w, float h);
    bool guiBlit(const char* path, float x, float y, float u, float v, float w, float h,
                 float texW = 256.0f, float texH = 256.0f, const glm::vec4& tint = glm::vec4(1.0f));
    bool guiSprite(const char* path, float x, float y, float w, float h,
                   const glm::vec4& tint = glm::vec4(1.0f));
    bool guiSpritePart(const char* path, float spriteW, float spriteH, float u, float v,
                       float x, float y, float w, float h);
    bool guiNineSlice(const char* path, float x, float y, float w, float h, float unit,
                      const glm::vec4& tint = glm::vec4(1.0f));
    bool guiHas(const char* path) { return m_gui.has(path); }
    bool guiTiled(const char* path, float x, float y, float w, float h, float tile,
                  const glm::vec4& tint = glm::vec4(1.0f));
    void menuBackdrop(float w, float h, float fallbackAlpha);
    void renderBossBars(float w);
    bool panoramaShowing();
    void renderPanorama();
    int m_pendingRenderDistance = 8;
    float m_vignette = 0.0f;
    struct BossBar { std::string name; int colour = 0; int notches = 0; float progress = 1.0f; };
    std::vector<BossBar> m_devBossBars;
    void drawBossBar(const BossBar& bar, float x, float y);
    void centredText(const std::string& text, float y, float scale, const glm::vec4& colour);
    void applyMenuAction();

    void setMouseCaptured(bool captured);
    bool playing() const { return m_screen == Screen::Playing; }
    bool uiHasFocus() const { return m_screen != Screen::Playing || m_inventoryOpen || m_chatOpen; }

    glm::vec3 sunDirection() const;
    float daylightFactor() const;
    // The same, plus whatever night vision is doing. For drawing
    // only: the mobs must never see it.
    float viewDaylight() const;

    // The Nether and the End have no sun, so they get a flat backdrop
    // and a raised light floor instead of a sky and a day cycle.
    bool hasSky() const { return m_dimension == Dimension::Overworld; }
    glm::vec3 dimensionBackdrop() const;
    float dimensionAmbient() const;

    // --- world lifecycle ---
    bool saveExists() const;
    void startWorld(GameMode mode, bool freshWorld);
    void quitToTitle();
    void deleteSavedWorld();
    void loadLevel();
    void saveLevel();

    // A real world generated behind the menus, with the camera slowly
    // orbiting it -- the same trick as Minecraft's title panorama, except
    // it's live terrain rather than a pre-rendered cubemap.
    // The player's name lives beside the save, so the browser keeps it in
    // IndexedDB along with the world.
    void loadProfile();
    void saveProfile();
    // Pushes the in-memory filesystem out to IndexedDB. Does nothing on
    // the desktop, where the files are already on disk.
    void flushSaveStorage();
    // Saves the bindings and makes sure they survive a page reload.
    void saveBindings();

    // Builds the world a host described, instead of the local save.
    void startJoinedWorld();

    // Rebuilds the world in another dimension, keeping the same save and
    // dropping the player somewhere they can stand.
    void travelTo(Dimension dimension);
    Dimension m_dimension = Dimension::Overworld;

    void startMenuWorld();
    void updateMenuCamera(float deltaTime);
    bool inGame() const
    {
        return m_screen == Screen::Playing || m_screen == Screen::Paused || m_screen == Screen::Dead;
    }

    // One clickable box in the open inventory. The list is rebuilt while
    // drawing and then reused for hit-testing, so what you see and what
    // you can click can never drift apart -- they used to be two separate
    // copies of the same geometry, and only one of them ever got fixed.
    struct SlotBox
    {
        enum class Kind { Slot, Palette, Tab, Enchant, Trade, AnvilTake, Destroy, Scroll, TradeTake, BeaconPower, BeaconConfirm, BeaconCancel, GrindTake, CutRecipe, CutTake, SmithTake, CartoTake, LoomPattern, LoomTake,
                          BookToggle, BookTab, BookFilter, BookPage, BookResult, BookSearch, BookAlt };
        Kind kind = Kind::Slot;
        int index = 0;        // inventory slot, block id, or tab number
        float x = 0.0f, y = 0.0f, size = 0.0f;

        // Most things on the panel are square, so `size` is both their
        // width and their height. A row is not: an offer at the
        // enchanting table or a villager's deal is five or six times as
        // wide as it is tall, and hit-testing it as a square left all
        // but the left-hand end of it dead to the mouse. Zero means
        // square, which is what every slot and palette entry is.
        float width = 0.0f;

        float hitWidth() const { return width > 0.0f ? width : size; }
    };

    struct HintBox
    {
        float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
        std::string text;
    };
    std::vector<HintBox> m_hintBoxes;
    void addHint(float x, float y, float w, float h, const std::string& text)
    {
        m_hintBoxes.push_back({ x, y, w, h, text });
    }
    std::vector<SlotBox> m_slotBoxes;
    bool m_recipeOpen = false;
    bool m_recipeFilter[4] = { false, false, false, false };
    int m_recipeTab = 0;
    int m_recipePage = 0;
    std::string m_recipeSearch;
    bool m_recipeSearching = false;
    int m_recipeGhost = -1;
    int m_recipeGhostBook = -1;
    int m_recipeAltSlot = -1;
    float m_recipeAltX = 0.0f;
    float m_recipeAltY = 0.0f;
    int bookKind() const;
    RecipeBook::Stock bookStock() const;
    std::vector<RecipeBook::Group> bookTabs() const;
    std::vector<RecipeBook::Slot> bookSlots() const;
    bool bookShown(float w) const;
    float bookPanelLeft(float w, float panelWidth) const;
    void takeForBook(StackId id, int count);
    void bookChoose(int index, bool all);
    void bookClick(const SlotBox& box, bool rightButton, bool shiftHeld);
    void renderRecipeBook(float w, float h, float panelX);
    void drawGhostRecipe(float panelX, float panelY);
    int m_creativeTab = 0;
    std::string m_creativeSearch;
    bool creativeSearching() const;

    const SlotBox* slotBoxAt(int mouseX, int mouseY) const;
};
