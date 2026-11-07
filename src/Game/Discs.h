#pragma once
#include "Items.h"

namespace Discs
{
    constexpr int COUNT = 19;

    struct Song
    {
        const char* key;
        const char* title;
        float seconds;
        int comparator;
    };

    const Song& song(int disc);
    int discOf(StackId id);
    StackId itemOf(int disc);
    bool creeperDrops(int disc);
}
