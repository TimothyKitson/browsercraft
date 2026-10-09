# Installs Minecraft entity textures into assets/skins/mob.
#
#   powershell -ExecutionPolicy Bypass -File tools\install-mob-textures.ps1 -Zip "C:\path\to\entity.zip"
#   powershell -ExecutionPolicy Bypass -File tools\install-mob-textures.ps1 -Folder "C:\...\textures\entity"
#
# The game draws its mobs on Mojang's own model layout, so a sheet from
# any pack that follows it lands correctly. Only the nine files the eight
# species need are copied; delete assets/skins/mob to go back to the
# hides the game paints for itself.
#
# Player skins come across too when the pack has them, into
# assets/skins/wide and assets/skins/slim, which is where the character
# picker looks before falling back to the characters it paints itself.
#
# These are not part of this project. assets/skins is gitignored, and
# vanilla entity textures are Mojang's -- they are fine on your own
# machine and are not yours to publish, which is why build-web.ps1 leaves
# them out unless you tell it otherwise.
param(
    [string]$Zip = "",
    [string]$Folder = "",
    [string]$Credit = ""
)

$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root "assets\skins\mob"

if (-not $Zip -and -not $Folder) {
    Write-Error "Pass -Zip <entity textures zip> or -Folder <textures\entity directory>"
    exit 1
}

# Each wanted sheet, and the names it goes by in a pack's entity folder.
$wanted = [ordered]@{
    "sheep.png"     = @("sheep/sheep.png", "sheep.png")
    "sheep_fur.png" = @("sheep/sheep_fur.png", "sheep_fur.png")
    "pig.png"       = @("pig/pig.png", "pig.png")
    "cow.png"       = @("cow/cow.png", "cow.png")
    "chicken.png"   = @("chicken.png", "chicken/chicken.png")
    "zombie.png"    = @("zombie/zombie.png", "zombie.png")
    "skeleton.png"  = @("skeleton/skeleton.png", "skeleton.png")
    "creeper.png"   = @("creeper/creeper.png", "creeper.png")
    "spider.png"    = @("spider/spider.png", "spider.png")
}

New-Item -ItemType Directory -Force -Path $dest | Out-Null

$source = $Folder
$temp = ""

if ($Zip) {
    if (-not (Test-Path -LiteralPath $Zip)) {
        Write-Error "Archive not found: $Zip"
        exit 1
    }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $temp = Join-Path ([System.IO.Path]::GetTempPath()) ("browsercraft-mobs-" + [guid]::NewGuid())
    New-Item -ItemType Directory -Force -Path $temp | Out-Null
    [System.IO.Compression.ZipFile]::ExtractToDirectory($Zip, $temp)
    $source = $temp
}

if (-not (Test-Path -LiteralPath $source)) {
    Write-Error "Folder not found: $source"
    exit 1
}

# A pack may hold the sheets at its root or buried under
# assets/minecraft/textures/entity, so every file is found by name rather
# than by a path guessed in advance.
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

Write-Output "Installed $copied of $($wanted.Count) mob textures into assets\skins\mob"
if ($missing.Count -gt 0) {
    Write-Warning ("Not found, so those species keep their painted hides: " + ($missing -join ", "))
}

# Player skins, if the pack carries them. Wide and slim are separate
# files: the arms are four pixels on one and three on the other. This has
# to happen before the extracted copy is thrown away, or every file it
# wants is already gone.
$skinRoot = Split-Path -Parent $dest
$players = 0

foreach ($build in @("wide", "slim")) {
    # Under player/ in a full entity pack, or at the root of one that
    # holds nothing but the player skins.
    $from = $files | Where-Object { $_.FullName -like "*\player\$build\*.png" }
    if (-not $from) {
        $from = $files | Where-Object { $_.FullName -like "*\$build\*.png" }
    }
    if (-not $from) { continue }

    $into = Join-Path $skinRoot $build
    New-Item -ItemType Directory -Force -Path $into | Out-Null
    foreach ($skin in $from) {
        $target = Join-Path $into $skin.Name
        Copy-Item -LiteralPath $skin.FullName -Destination $target -Force -ErrorAction SilentlyContinue
        # Counted only once it is actually there, so a failed copy cannot
        # report itself as an install.
        if (Test-Path -LiteralPath $target) { $players++ }
    }
}

if ($players -gt 0) {
    Write-Output "Installed $players player skins into assets\skins\wide and assets\skins\slim"
} else {
    Write-Output "No player skins in this pack - the painted characters stay."
}

if ($temp) { Remove-Item -LiteralPath $temp -Recurse -Force }

if ($Credit) {
    Set-Content -LiteralPath (Join-Path $dest "CREDIT.txt") -Value $Credit -Encoding UTF8
    Write-Output "Credited to: $Credit"
}
