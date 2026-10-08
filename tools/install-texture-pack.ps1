# Installs a Minecraft resource pack into assets/textures/block.
#
#   powershell -ExecutionPolicy Bypass -File tools\install-texture-pack.ps1 -Zip "C:\path\to\pack.zip"
#
# Only the textures this game actually uses are copied, along with any
# LabPBR "_n" (normal) and "_s" (specular) maps the pack ships. Delete the
# assets/textures/block folder to go back to the built-in procedural
# textures.
#
# Most packs are free to pass on only if whoever made them is credited.
# -Credit writes assets/textures/CREDIT.txt, which the game shows in the
# corner of the title screen. The credit lives next to the art rather
# than in the source, because the repository never contains either.
param(
    [Parameter(Mandatory = $true)][string]$Zip,
    [string]$Credit = "",
    [switch]$NoPbr
)

$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root "assets\textures\block"

if (-not (Test-Path -LiteralPath $Zip)) {
    Write-Error "Pack not found: $Zip"
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
New-Item -ItemType Directory -Force -Path $dest | Out-Null

# Vanilla names the engine looks for, newest-first alternatives included.
$wanted = @(
    'stone','dirt','grass_block_top','grass_block_side','grass_block_side_overlay','cobblestone',
    'oak_planks','planks_oak','bedrock','water_still','sand','gravel','gold_ore','iron_ore',
    'coal_ore','diamond_ore','oak_log','log_oak','oak_log_top','log_oak_top','oak_leaves',
    'leaves_oak','glass','sandstone','sandstone_normal','sandstone_top','snow','ice','cactus_side',
    'cactus_top','bricks','brick','obsidian','short_grass','grass','tallgrass','poppy','flower_rose',
    'dandelion','flower_dandelion','mossy_cobblestone','cobblestone_mossy','clay','pumpkin_side',
    'pumpkin_top','white_wool','wool_colored_white','torch','torch_on','birch_log','log_birch',
    'birch_log_top','log_birch_top','birch_leaves','leaves_birch','grass_block_snow',
    'grass_side_snowed','lava_still','glowstone'
)
0..9 | ForEach-Object { $wanted += "destroy_stage_$_" }

$archive = [System.IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Zip))

# Resource packs put block textures in textures/block (1.13+) or
# textures/blocks (1.12 and earlier).
$entries = @{}
foreach ($e in $archive.Entries) {
    if ($e.FullName -match 'textures/blocks?/([^/]+\.png)$') { $entries[$matches[1]] = $e }
}

$copied = 0; $pbr = 0; $resolution = 0
foreach ($name in $wanted) {
    $variants = @("$name.png")
    if (-not $NoPbr) { $variants += @("${name}_n.png", "${name}_s.png") }

    foreach ($file in $variants) {
        if (-not $entries.ContainsKey($file)) { continue }
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entries[$file], (Join-Path $dest $file), $true)
        if ($file -like "*_n.png" -or $file -like "*_s.png") { $pbr++ } else { $copied++ }
    }
}
# HUD sprites (hearts, hotbar). Missing ones fall back to procedural art.
$guiDest = Join-Path $root "assets\textures\gui"
New-Item -ItemType Directory -Force -Path $guiDest | Out-Null
$guiMap = @{
    "gui/sprites/hud/heart/full.png"               = "heart_full.png"
    "gui/sprites/hud/heart/half.png"               = "heart_half.png"
    "gui/sprites/hud/heart/container.png"          = "heart_container.png"
    "gui/sprites/hud/heart/hardcore_full.png"      = "heart_hardcore_full.png"
    "gui/sprites/hud/heart/hardcore_half.png"      = "heart_hardcore_half.png"
    "gui/sprites/hud/heart/container_hardcore.png" = "heart_hardcore_container.png"
    "gui/sprites/hud/hotbar.png"                   = "hotbar.png"
    "gui/sprites/hud/hotbar_selection.png"         = "hotbar_selection.png"
}
$guiFound = 0
foreach ($entry in $archive.Entries) {
    foreach ($key in $guiMap.Keys) {
        if ($entry.FullName -like "*$key") {
            [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, (Join-Path $guiDest $guiMap[$key]), $true)
            $guiFound++
            break
        }
    }
}

$archive.Dispose()

# The credit travels with the art, so installing a different pack cannot
# leave the last one's name on the title screen.
$creditPath = Join-Path $root "assets\textures\CREDIT.txt"
if ($Credit -ne "") {
    Set-Content -LiteralPath $creditPath -Value $Credit -Encoding UTF8
    Write-Output "Credited as: $Credit"
} elseif (Test-Path -LiteralPath $creditPath) {
    Remove-Item -LiteralPath $creditPath
    Write-Output "No -Credit given, so the old CREDIT.txt was removed."
} else {
    Write-Output "No -Credit given. Check whether this pack requires attribution."
}

# Report the tile resolution so you know what you installed.
$probe = Join-Path $dest "stone.png"
if (Test-Path -LiteralPath $probe) {
    Add-Type -AssemblyName System.Drawing
    $img = New-Object System.Drawing.Bitmap($probe)
    $resolution = $img.Width
    $img.Dispose()
}

Write-Output "Installed $copied block textures and $pbr PBR maps into assets/textures/block"
Write-Output "Installed $guiFound HUD sprites into assets/textures/gui"
if ($resolution -gt 0) { Write-Output "Tile resolution: ${resolution}x${resolution}" }
if ($resolution -ge 128) {
    Write-Output "Note: at ${resolution}x the atlas is $(16 * $resolution)px square per layer (albedo + normal + specular)."
    Write-Output "      That is a lot of VRAM and a very large download if you ship the web build."
}
Write-Output "Rebuild (build.bat) so the assets folder is copied next to the exe."
