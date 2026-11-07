#include "Discs.h"
#include <algorithm>

namespace
{
    const Discs::Song SONGS[Discs::COUNT] = {
        { "13", "C418 - 13", 178.0f, 1 },
        { "cat", "C418 - cat", 185.0f, 2 },
        { "blocks", "C418 - blocks", 345.0f, 3 },
        { "chirp", "C418 - chirp", 185.0f, 4 },
        { "far", "C418 - far", 174.0f, 5 },
        { "mall", "C418 - mall", 197.0f, 6 },
        { "mellohi", "C418 - mellohi", 96.0f, 7 },
        { "stal", "C418 - stal", 150.0f, 8 },
        { "strad", "C418 - strad", 188.0f, 9 },
        { "ward", "C418 - ward", 251.0f, 10 },
        { "11", "C418 - 11", 71.0f, 11 },
        { "wait", "C418 - wait", 238.0f, 12 },
        { "otherside", "Lena Raine - otherside", 195.0f, 14 },
        { "5", "Samuel Aberg - 5", 178.0f, 15 },
        { "pigstep", "Lena Raine - Pigstep", 149.0f, 13 },
        { "relic", "Aaron Cherof - Relic", 218.0f, 14 },
        { "creator", "Lena Raine - Creator", 176.0f, 12 },
        { "creator_music_box", "Lena Raine - Creator (Music Box)", 73.0f, 11 },
        { "precipice", "Aaron Cherof - Precipice", 299.0f, 13 },
    };
}

namespace Discs
{
    const Song& song(int disc) { return SONGS[std::clamp(disc, 0, COUNT - 1)]; }

    int discOf(StackId id)
    {
        if (id < Items::MusicDiscFirst || id > Items::MusicDiscLast) return -1;
        return static_cast<int>(id - Items::MusicDiscFirst);
    }

    StackId itemOf(int disc) { return static_cast<StackId>(Items::MusicDiscFirst + std::clamp(disc, 0, COUNT - 1)); }

    bool creeperDrops(int disc) { return disc >= 0 && disc <= 11; }
}
