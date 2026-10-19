#!/usr/bin/env bash
#
# Installs a vanilla Minecraft texture set -- the assets/minecraft/textures
# tree out of a client jar, or one of the extracted dumps of it -- as this
# game's texture pack.
#
#   tools/install-vanilla-textures.sh ~/Downloads/1.21/textures
#
# What goes where: block/ and item/ are the atlas, gui/ is the HUD, and
# the entity sheets are picked out one at a time, because this engine
# names its mobs after the species rather than after Mojang's file paths.
#
# Mojang's textures are Mojang's. They stay out of the repository -- the
# three asset folders are gitignored for exactly this reason -- and out
# of any build that goes on a public site.
set -euo pipefail

if [ $# -lt 1 ]; then
    echo "usage: $(basename "$0") <path to assets/minecraft/textures>" >&2
    exit 1
fi

src="$1"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

for want in block item entity gui; do
    [ -d "$src/$want" ] || { echo "$src has no $want/ -- is that the textures folder?" >&2; exit 1; }
done

# Anything already installed is kept, under a name that says what it was,
# so switching back to it is a rename rather than another download.
stamp="$(date +%Y%m%d-%H%M%S)"
for dir in textures/block textures/item textures/gui skins/mob; do
    if [ -d "$root/assets/$dir" ] && [ -n "$(ls -A "$root/assets/$dir" 2>/dev/null)" ]; then
        mv "$root/assets/$dir" "$root/assets/$dir.$stamp"
        echo "Kept the old $dir as $dir.$stamp"
    fi
done

mkdir -p "$root/assets/textures/block" "$root/assets/textures/item" \
         "$root/assets/textures/gui" "$root/assets/skins/mob"

cp "$src"/block/*.png "$root/assets/textures/block/"
cp "$src"/item/*.png "$root/assets/textures/item/"
echo "Blocks: $(ls "$root/assets/textures/block" | wc -l), items: $(ls "$root/assets/textures/item" | wc -l)"

# The HUD. Vanilla files are named by what they are inside a sprite
# folder; this engine names them by what they are full stop.
gui() {
    [ -f "$src/$1" ] && cp "$src/$1" "$root/assets/textures/gui/$2"
}
gui gui/sprites/hud/heart/full.png              heart_full.png
gui gui/sprites/hud/heart/half.png              heart_half.png
gui gui/sprites/hud/heart/container.png         heart_container.png
gui gui/sprites/hud/heart/hardcore_full.png     heart_hardcore_full.png
gui gui/sprites/hud/heart/hardcore_half.png     heart_hardcore_half.png
gui gui/sprites/hud/heart/container_hardcore.png heart_hardcore_container.png
gui gui/sprites/hud/food_full.png               food_full.png
gui gui/sprites/hud/food_half.png               food_half.png
gui gui/sprites/hud/food_empty.png              food_empty.png
gui gui/sprites/hud/armor_full.png              armor_full.png
gui gui/sprites/hud/armor_half.png              armor_half.png
gui gui/sprites/hud/armor_empty.png             armor_empty.png
gui gui/sprites/hud/hotbar.png                  hotbar.png
gui gui/sprites/hud/hotbar_selection.png        hotbar_selection.png
for piece in helmet chestplate leggings boots; do
    gui "item/empty_armor_slot_$piece.png" "empty_armor_slot_$piece.png"
done
echo "HUD: $(ls "$root/assets/textures/gui" | wc -l) sprites"

# The mobs, one at a time. A species whose sheet is not where this says
# falls back to the hide the game paints for itself, which is why every
# line here is allowed to miss.
mob() {
    [ -f "$src/entity/$1" ] && cp "$src/entity/$1" "$root/assets/skins/mob/$2.png"
}
mob sheep/sheep.png                 sheep
mob sheep/sheep_fur.png             sheep_fur
mob pig/pig.png                     pig
mob cow/cow.png                     cow
mob chicken.png                     chicken
mob zombie/zombie.png               zombie
mob skeleton/skeleton.png           skeleton
mob skeleton/wither_skeleton.png    wither_skeleton
mob creeper/creeper.png             creeper
mob spider/spider.png               spider
mob enderman/enderman.png           enderman
mob villager/villager.png           villager
mob wolf/wolf.png                   wolf
mob horse/horse_brown.png           horse
mob boat/oak.png                    boat
mob minecart.png                    minecart
mob iron_golem/iron_golem.png       iron_golem
mob blaze.png                       blaze
mob slime/magmacube.png             magmacube
mob breeze/breeze.png               breeze

# 1.21 has no zombie pigman: it was replaced by the zombified piglin,
# which is a piglin and not a zombie. The body, arms and legs are the
# same biped patches, so the sheet reads correctly on this engine's
# model; what is lost is the nose and ears, which it does not draw.
mob piglin/zombified_piglin.png     zombie_pigman

[ -f "$src/entity/shield/base.png" ] && cp "$src/entity/shield/base.png" "$root/assets/skins/shield.png"
echo "Mobs: $(ls "$root/assets/skins/mob"/*.png 2>/dev/null | wc -l) sheets"

printf 'Minecraft 1.21 (Mojang Studios) -- not for redistribution\n' > "$root/assets/textures/CREDIT.txt"
printf 'Minecraft 1.21 (Mojang Studios) -- not for redistribution\n' > "$root/assets/skins/mob/CREDIT.txt"
echo "Installed. Run with --textures-info to see what the atlas picked up."
