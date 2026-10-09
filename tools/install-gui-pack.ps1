# Installs a Minecraft GUI pack into assets/textures/gui.
#
#   powershell -ExecutionPolicy Bypass -File tools\install-gui-pack.ps1 -Zip "C:\path\to\gui.zip"
#   powershell -ExecutionPolicy Bypass -File tools\install-gui-pack.ps1 -Folder "C:\...\textures\gui"
#
# Modern Minecraft keeps the HUD as one sprite per file under
# sprites/hud, while this engine asks for a flat set of names. The table
# below maps between them, and accepts the flat names too, so a pack
# laid out either way drops in.
#
# Anything missing keeps the sprite the game draws for itself, so a
# partial pack is fine. Delete assets/textures/gui to go back.
#
# These are not part of this project. Minecraft's GUI art is Mojang's:
# fine on your own machine, not yours to publish.
param(
    [string]$Zip = "",
    [string]$Folder = "",
    [string]$Credit = ""
)

$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root "assets\textures\gui"

if (-not $Zip -and -not $Folder) {
    Write-Error "Pass -Zip <gui pack zip> or -Folder <a textures\gui directory>"
    exit 1
}

# What the engine loads, and where a pack might keep it. First match wins.
$wanted = [ordered]@{
    "heart_full.png"               = @("sprites/hud/heart/full.png", "heart_full.png")
    "heart_half.png"               = @("sprites/hud/heart/half.png", "heart_half.png")
    "heart_container.png"          = @("sprites/hud/heart/container.png", "heart_container.png")
    "heart_hardcore_full.png"      = @("sprites/hud/heart/hardcore_full.png", "heart_hardcore_full.png")
    "heart_hardcore_half.png"      = @("sprites/hud/heart/hardcore_half.png", "heart_hardcore_half.png")
    "heart_hardcore_container.png" = @("sprites/hud/heart/container_hardcore.png", "heart_hardcore_container.png")
    "food_full.png"                = @("sprites/hud/food_full.png", "food_full.png")
    "food_half.png"                = @("sprites/hud/food_half.png", "food_half.png")
    "food_empty.png"               = @("sprites/hud/food_empty.png", "food_empty.png")
    "hotbar.png"                   = @("sprites/hud/hotbar.png", "hotbar.png")
    "hotbar_selection.png"         = @("sprites/hud/hotbar_selection.png", "hotbar_selection.png")
    "empty_armor_slot_helmet.png"     = @("sprites/container/slot/helmet.png", "empty_armor_slot_helmet.png")
    "empty_armor_slot_chestplate.png" = @("sprites/container/slot/chestplate.png", "empty_armor_slot_chestplate.png")
    "empty_armor_slot_leggings.png"   = @("sprites/container/slot/leggings.png", "empty_armor_slot_leggings.png")
    "empty_armor_slot_boots.png"      = @("sprites/container/slot/boots.png", "empty_armor_slot_boots.png")
}

$source = $Folder
$temp = ""

if ($Zip) {
    if (-not (Test-Path -LiteralPath $Zip)) {
        Write-Error "Archive not found: $Zip"
        exit 1
    }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $temp = Join-Path ([System.IO.Path]::GetTempPath()) ("browsercraft-gui-" + [guid]::NewGuid())
    New-Item -ItemType Directory -Force -Path $temp | Out-Null
    [System.IO.Compression.ZipFile]::ExtractToDirectory($Zip, $temp)
    $source = $temp
}

if (-not (Test-Path -LiteralPath $source)) {
    Write-Error "Folder not found: $source"
    exit 1
}

New-Item -ItemType Directory -Force -Path $dest | Out-Null
$files = Get-ChildItem -LiteralPath $source -Recurse -Filter *.png -File

$copied = 0
$missing = @()

foreach ($name in $wanted.Keys) {
    $found = $null
    foreach ($candidate in $wanted[$name]) {
        $tail = $candidate.Replace('/', '\')
        $found = $files | Where-Object { $_.FullName -like "*\$tail" } | Select-Object -First 1
        if ($found) { break }
    }

    if ($found) {
        Copy-Item -LiteralPath $found.FullName -Destination (Join-Path $dest $name) -Force
        $copied++
    }
    else {
        $missing += $name
    }
}

if ($temp) { Remove-Item -LiteralPath $temp -Recurse -Force }

Write-Output "Installed $copied of $($wanted.Count) GUI sprites into assets\textures\gui"
if ($missing.Count -gt 0) {
    Write-Warning ("Not in this pack, so those stay as the game draws them: " + ($missing -join ", "))
}

if ($Credit) {
    Set-Content -LiteralPath (Join-Path $dest "CREDIT.txt") -Value $Credit -Encoding UTF8
    Write-Output "Credited to: $Credit"
}

Write-Output ""
Write-Output "Rebuild (.\build.bat) or copy assets\textures\gui into build\assets\textures\ to see them."
