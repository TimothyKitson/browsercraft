# Installs a Minecraft sound pack into assets/sounds.
#
#   powershell -ExecutionPolicy Bypass -File tools\install-sound-pack.ps1 -Zip "C:\path\to\sounds.zip"
#   powershell -ExecutionPolicy Bypass -File tools\install-sound-pack.ps1 -Folder "C:\...\assets\minecraft\sounds"
#
# The engine asks for Minecraft's own sound names, so a pack laid out the
# vanilla way drops straight in: dig/stone1.ogg, step/grass1.ogg,
# mob/cow/say1.ogg, random/pop.ogg and so on. Anything it cannot find
# keeps the voice the game synthesises for itself, so a partial pack is
# fine and still sounds right.
#
# Delete assets/sounds to go back to synthesised audio everywhere.
#
# These are not part of this project. assets/sounds is gitignored, and
# Minecraft's own audio is Mojang's: fine on your own machine, not yours
# to publish. build-web.ps1 leaves it out of the download for that reason.
param(
    [string]$Zip = "",
    [string]$Folder = "",
    [string]$Credit = ""
)

$root = Split-Path -Parent $PSScriptRoot
$dest = Join-Path $root "assets\sounds"

if (-not $Zip -and -not $Folder) {
    Write-Error "Pass -Zip <sound pack zip> or -Folder <a sounds directory>"
    exit 1
}

$source = $Folder
$temp = ""

if ($Zip) {
    if (-not (Test-Path -LiteralPath $Zip)) {
        Write-Error "Archive not found: $Zip"
        exit 1
    }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $temp = Join-Path ([System.IO.Path]::GetTempPath()) ("browsercraft-sounds-" + [guid]::NewGuid())
    New-Item -ItemType Directory -Force -Path $temp | Out-Null
    [System.IO.Compression.ZipFile]::ExtractToDirectory($Zip, $temp)
    $source = $temp
}

if (-not (Test-Path -LiteralPath $source)) {
    Write-Error "Folder not found: $source"
    exit 1
}

# A pack may hold the clips at its root or buried under
# assets/minecraft/sounds, so the folder that actually holds the vanilla
# subfolders is found rather than guessed.
$anchor = $null
foreach ($marker in @("dig", "step", "mob", "random")) {
    $found = Get-ChildItem -LiteralPath $source -Recurse -Directory -Filter $marker -ErrorAction SilentlyContinue |
             Select-Object -First 1
    if ($found) { $anchor = $found.Parent.FullName; break }
}
if (-not $anchor) { $anchor = $source }

New-Item -ItemType Directory -Force -Path $dest | Out-Null

$clips = Get-ChildItem -LiteralPath $anchor -Recurse -Filter *.ogg -File
if ($clips.Count -eq 0) {
    Write-Error "No .ogg files under $anchor - is this a sound pack?"
    exit 1
}

$copied = 0
$skipped = @()

foreach ($clip in $clips) {
    # The engine decodes Ogg Vorbis and nothing else. A mislabelled mp3
    # would install silently and play as silence, so it is turned away.
    $head = [System.IO.File]::ReadAllBytes($clip.FullName)[0..3]
    if (-not ($head[0] -eq 0x4F -and $head[1] -eq 0x67 -and $head[2] -eq 0x67 -and $head[3] -eq 0x53)) {
        $skipped += $clip.Name
        continue
    }

    $relative = $clip.FullName.Substring($anchor.Length).TrimStart('\')
    $target = Join-Path $dest $relative
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
    Copy-Item -LiteralPath $clip.FullName -Destination $target -Force
    $copied++
}

if ($temp) { Remove-Item -LiteralPath $temp -Recurse -Force }

Write-Output "Installed $copied clips into assets\sounds"
if ($skipped.Count -gt 0) {
    Write-Warning ("Not Ogg Vorbis, so left out: " + ($skipped -join ", "))
}

if ($Credit) {
    # WriteAllText rather than Set-Content: Windows PowerShell's UTF8
    # encoding writes a byte-order mark, which the game then showed as
    # three stray characters in front of the name.
    [System.IO.File]::WriteAllText((Join-Path $dest "CREDIT.txt"), $Credit,
                                   (New-Object System.Text.UTF8Encoding($false)))
    Write-Output "Credited to: $Credit"
}

Write-Output ""
Write-Output "Rebuild (.\build.bat) or copy assets\sounds into build\assets\ to hear them."
