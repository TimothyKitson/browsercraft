#pragma once
#include "Items.h"
#include "World/Block.h"
#include <vector>

namespace Stonecutting
{
    struct Option
    {
        StackId result = Blocks::Air;
        int count = 0;
    };

    std::vector<Option> optionsFor(StackId input);
    std::vector<StackId> everyOutput();
}
