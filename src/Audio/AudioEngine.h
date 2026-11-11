#pragma once
#include "World/Block.h"
#include <vector>
#include <array>
#include <string>
#include <mutex>
#include <cstdint>

enum class Sound : uint16_t
{
    DigStone,
    DigDirt,
    DigGrass,
    DigWood,
    DigSand,
    DigGlass,
    DigWool,
    DigPlant,
    StepStone,
    StepGravel,
    StepGrass,
    StepWood,
    StepSand,
    StepCloth,
    StepSnow,
    Place,
    Jump,
    Land,
    Hurt,
    Pickup,
    Click,
    MobSheepSay,
    MobSheepHurt,
    MobSheepDeath,
    MobPigSay,
    MobPigHurt,
    MobPigDeath,
    MobCowSay,
    MobCowHurt,
    MobCowDeath,
    MobChickenSay,
    MobChickenHurt,
    MobChickenDeath,
    MobZombieSay,
    MobZombieHurt,
    MobZombieDeath,
    MobSkeletonSay,
    MobSkeletonHurt,
    MobSkeletonDeath,
    MobCreeperSay,
    MobCreeperHurt,
    MobCreeperDeath,
    MobSpiderSay,
    MobSpiderHurt,
    MobSpiderDeath,
    MobEndermanSay,
    MobEndermanHurt,
    MobEndermanDeath,
    MobPigmanSay,
    MobPigmanHurt,
    MobPigmanDeath,
    MobVillagerSay,
    MobVillagerHurt,
    MobVillagerDeath,
    MobWolfSay,
    MobWolfHurt,
    MobWolfDeath,
    MobHorseSay,
    MobHorseHurt,
    MobHorseDeath,
    MobGolemSay,
    MobGolemHurt,
    MobGolemDeath,
    MobBlazeSay,
    MobBlazeHurt,
    MobBlazeDeath,
    MobWitherSkeletonSay,
    MobWitherSkeletonHurt,
    MobWitherSkeletonDeath,
    MobMagmaCubeSay,
    MobMagmaCubeHurt,
    MobMagmaCubeDeath,
    MobWitherSay,
    MobWitherHurt,
    MobWitherDeath,
    MobDragonSay,
    MobDragonHurt,
    MobDragonDeath,
    MobWitchSay,
    MobWitchHurt,
    MobWitchDeath,
    MobGuardianSay,
    MobGuardianHurt,
    MobGuardianDeath,
    MobWardenSay,
    MobWardenHurt,
    MobWardenDeath,
    MobBreezeSay,
    MobBreezeHurt,
    MobBreezeDeath,
    MobShulkerSay,
    MobShulkerHurt,
    MobShulkerDeath,
    MobDrownedSay,
    MobDrownedHurt,
    MobDrownedDeath,
    MobFrogSay,
    MobFrogHurt,
    MobFrogDeath,
    MobSquidSay,
    MobSquidHurt,
    MobSquidDeath,
    MobGhastSay,
    MobGhastHurt,
    MobGhastDeath,
    MobPillagerSay,
    MobPillagerHurt,
    MobPillagerDeath,
    MobVindicatorSay,
    MobVindicatorHurt,
    MobVindicatorDeath,
    MobEvokerSay,
    MobEvokerHurt,
    MobEvokerDeath,
    MobRavagerSay,
    MobRavagerHurt,
    MobRavagerDeath,
    MobSlimeSay,
    MobSlimeHurt,
    MobSlimeDeath,
    MobSlimeSmallSay,
    MobSlimeSmallHurt,
    MobSlimeSmallDeath,
    MobBatSay,
    MobBatHurt,
    MobBatDeath,
    MobCaveSpiderSay,
    MobCaveSpiderHurt,
    MobCaveSpiderDeath,
    MobSilverfishSay,
    MobSilverfishHurt,
    MobSilverfishDeath,
    MobPiglinSay,
    MobPiglinHurt,
    MobPiglinDeath,
    MobPiglinBruteSay,
    MobPiglinBruteHurt,
    MobPiglinBruteDeath,
    MobHoglinSay,
    MobHoglinHurt,
    MobHoglinDeath,
    MobStriderSay,
    MobStriderHurt,
    MobStriderDeath,
    MobCodSay,
    MobCodHurt,
    MobCodDeath,
    MobSalmonSay,
    MobSalmonHurt,
    MobSalmonDeath,
    MobPufferfishSay,
    MobPufferfishHurt,
    MobPufferfishDeath,
    MobDolphinSay,
    MobDolphinHurt,
    MobDolphinDeath,
    MobTurtleSay,
    MobTurtleHurt,
    MobTurtleDeath,
    MobAxolotlSay,
    MobAxolotlHurt,
    MobAxolotlDeath,
    MobRabbitSay,
    MobRabbitHurt,
    MobRabbitDeath,
    MobFoxSay,
    MobFoxHurt,
    MobFoxDeath,
    MobGoatSay,
    MobGoatHurt,
    MobGoatDeath,
    MobPandaSay,
    MobPandaHurt,
    MobPandaDeath,
    MobBeeSay,
    MobBeeHurt,
    MobBeeDeath,
    MobCatSay,
    MobCatHurt,
    MobCatDeath,
    MobLlamaSay,
    MobLlamaHurt,
    MobLlamaDeath,
    MobPolarBearSay,
    MobPolarBearHurt,
    MobPolarBearDeath,
    MobStraySay,
    MobStrayHurt,
    MobStrayDeath,
    MobHuskSay,
    MobHuskHurt,
    MobHuskDeath,
    MobBoggedSay,
    MobBoggedHurt,
    MobBoggedDeath,
    MobPhantomSay,
    MobPhantomHurt,
    MobPhantomDeath,
    MobZombieVillagerSay,
    MobZombieVillagerHurt,
    MobZombieVillagerDeath,
    MobEndermiteSay,
    MobEndermiteHurt,
    MobEndermiteDeath,
    MobVexSay,
    MobVexHurt,
    MobVexDeath,
    MobSnowGolemSay,
    MobSnowGolemHurt,
    MobSnowGolemDeath,
    MobElderGuardianSay,
    MobElderGuardianHurt,
    MobElderGuardianDeath,
    MobTraderSay,
    MobTraderHurt,
    MobTraderDeath,
    MobCamelSay,
    MobCamelHurt,
    MobCamelDeath,
    MobParrotSay,
    MobParrotHurt,
    MobParrotDeath,
    MobAllaySay,
    MobAllayHurt,
    MobAllayDeath,
    MobSnifferSay,
    MobSnifferHurt,
    MobSnifferDeath,
    MobTropicalFishSay,
    MobTropicalFishHurt,
    MobTropicalFishDeath,
    MobTadpoleSay,
    MobTadpoleHurt,
    MobTadpoleDeath,
    MobArmadilloSay,
    MobArmadilloHurt,
    MobArmadilloDeath,
    MobGlowSquidSay,
    MobGlowSquidHurt,
    MobGlowSquidDeath,
    MobZoglinSay,
    MobZoglinHurt,
    MobZoglinDeath,
    MobIllusionerSay,
    MobIllusionerHurt,
    MobIllusionerDeath,
    Rain,
    Thunder,

    // Fire: a blaze loosing a charge, and the player alight.
    Fire,
    MobStep,
    SculkClick,
    FireworkLaunch,
    FireworkBlast,
    FireworkLargeBlast,
    FireworkTwinkle,
    DyeUse,
    InkUse,
    GlowInkUse,
    EvokerFangs,
    EvokerCast,
    IllusionerMirror,
    IllusionerCast,
    ZoglinConvert,
    LlamaSpit,
    WindBurst,
    RaidHorn,
    OmenBad,
    OmenTrial,
    OmenRaid,
    ElderCurse,
    Shriek,
    ToastIn,
    ToastOut,
    ChallengeComplete,
    GoatHornFirst,
    GoatHornLast = GoatHornFirst + 7,
    MobDonkeySay,
    MobDonkeyHurt,
    MobDonkeyDeath,
    MobMuleSay,
    MobMuleHurt,
    MobMuleDeath,
    MobSkeletonHorseSay,
    MobSkeletonHorseHurt,
    MobSkeletonHorseDeath,
    MobZombieHorseSay,
    MobZombieHorseHurt,
    MobZombieHorseDeath,
    MobOcelotSay,
    MobOcelotHurt,
    MobOcelotDeath,
    NoteFirst,
    NoteLast = NoteFirst + 15,
    Count
};

// Sounds come from assets/sounds when that folder exists, and are
// synthesised otherwise.
//
// Same arrangement as the textures: real Minecraft audio is Mojang's and
// cannot ship in a public build, but nothing stops you dropping it in on
// your own machine. The synthesised set is what browsercraft.net serves,
// and it means the game is never silent.
//
// Each sound holds several variants, because vanilla has four or five
// takes of every footstep and picking between them is most of why
// walking does not sound like a machine.
class AudioEngine
{
public:
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    void play(Sound sound, float volume = 1.0f, float pitch = 1.0f);

    // Convenience wrappers that pick the right sound for a block.
    void playDig(BlockId block, float volume = 1.0f);
    void playStep(BlockId block);
    void playPlace(BlockId block);

    void setMuted(bool muted) { m_muted = muted; }
    void setMasterVolume(float volume) { m_master = volume < 0.0f ? 0.0f : volume > 1.0f ? 1.0f : volume; }
    bool muted() const { return m_muted; }
    bool available() const { return m_device != 0; }

    // How many clips a sound has, loaded or synthesised. Exposed only
    // so --selftest can sweep the whole Sound enum and catch one that
    // was added without anything to play for it: a silent sound is not
    // an error anywhere, it just never makes a noise.
    int clipCount(Sound sound) const;

    bool playMusic(const std::string& file, uint32_t tuneSeed, float seconds);
    void stopMusic();
    void setMusicVolume(float volume);
    bool musicPlaying();

private:
    static constexpr int SAMPLE_RATE = 44100;
    static constexpr int MAX_VOICES = 24;

    struct Voice
    {
        const std::vector<float>* buffer = nullptr;
        double position = 0.0;
        double step = 1.0;
        float volume = 1.0f;
    };

    unsigned int m_device = 0;
    bool m_muted = false;
    float m_master = 1.0f;
    using Clip = std::vector<float>;
    std::array<std::vector<Clip>, static_cast<size_t>(Sound::Count)> m_buffers;
    int m_loadedFromFiles = 0;
    // Somewhere harmless for the synth to write when a sound already came
    // from a file and the synthesised version is not wanted.
    Clip m_scratch;
    std::array<Voice, MAX_VOICES> m_voices{};
    std::mutex m_mutex;

    void buildSounds();
    // Decodes one Ogg Vorbis file to mono at the mixer's sample rate.
    static bool loadOgg(const std::string& path, Clip& out);
    // Loads "<stem>1.ogg", "<stem>2.ogg", ... until one is missing.
    int loadVariants(Sound id, const char* stem, int maxVariants);
    bool loadSingle(Sound id, const char* file);

    // The shortest gap allowed between two plays of the same sound. A
    // call site that fires every frame would otherwise stack dozens of
    // copies of one clip into a single noise.
    static constexpr float RETRIGGER_SECONDS = 0.06f;
    std::array<uint32_t, static_cast<size_t>(Sound::Count)> m_lastPlayed{};
    void loadMobVoice(Sound say, Sound hurt, Sound death, const char* folder);
    void mix(float* output, int frames);
    struct Music
    {
        void* stream = nullptr;
        int channels = 0;
        int rate = 0;
        std::vector<float> decoded;
        long bufferStart = 0;
        long bufferFrames = 0;
        double position = 0.0;
        Clip tune;
        double tunePosition = 0.0;
        double secondsLeft = 0.0;
        float volume = 0.0f;
        bool active = false;
    } m_music;
    void closeMusicStream();
    void mixMusic(float* output, int frames);
    static void callback(void* userData, uint8_t* stream, int lengthBytes);
};
