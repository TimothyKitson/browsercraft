#include "Golems.h"

MobId Golems::madeBy(const Shape& shape)
{
    if (shape.upper == Blocks::Snow && shape.lower == Blocks::Snow)
        return MobId::SnowGolem;

    if (shape.upper == Blocks::IronBlock && shape.lower == Blocks::IronBlock &&
        shape.armA == Blocks::IronBlock && shape.armB == Blocks::IronBlock)
        return MobId::IronGolem;

    return MobId::Count;
}
