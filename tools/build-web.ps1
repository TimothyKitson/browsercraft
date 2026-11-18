# Builds the browser (WebAssembly) version into build-web/.
#
#   powershell -ExecutionPolicy Bypass -File tools\build-web.ps1
#   powershell -ExecutionPolicy Bypass -File tools\build-web.ps1 -Serve
#
# The web build deliberately ships a lower-resolution texture set: every
# texture is baked into the page download, so a 256x pack would mean tens
# of megabytes before anyone sees a single block.
param(
    [string]$Emsdk = "C:\Users\TempAdmin\emsdk",
    [string]$TextureDir = "",   # defaults to assets/textures/block (the installed pack)
    [switch]$MobTextures,       # include assets/skins/mob when redistribution is permitted
    [switch]$SoundFiles,        # include assets/sounds when redistribution is permitted
    [switch]$Serve,
    [int]$Port = 8080
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (-not (Test-Path -LiteralPath (Join-Path $Emsdk "emsdk_env.bat"))) {
    Write-Error "Emscripten not found at $Emsdk. Clone https://github.com/emscripten-core/emsdk and run 'emsdk install latest; emsdk activate latest'."
    exit 1
}

# Stage every asset category the runtime knows how to load. Copy the
# contents of each source directory (not the source directory itself), so
# the runtime paths remain assets/textures/block, item, gui and entity.
function Copy-DirectoryContents {
    param(
        [Parameter(Mandatory = $true)][string]$Source,
        [Parameter(Mandatory = $true)][string]$Destination
    )
    if (-not (Test-Path -LiteralPath $Source -PathType Container)) { return $false }
    New-Item -ItemType Directory -Force -Path $Destination | Out-Null
    Get-ChildItem -LiteralPath $Source -Force | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination $Destination -Recurse -Force
    }
    return $true
}

$stage = Join-Path $root "build-web\stage\assets"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -Recurse (Join-Path $root "assets\shaders") (Join-Path $stage "shaders")

if ($TextureDir -eq "") {
    $lowRes = Join-Path $root "assets\textures\block32"
    $normal = Join-Path $root "assets\textures\block"
    if (Test-Path $lowRes -PathType Container) { $TextureDir = $lowRes }
    elseif (Test-Path $normal -PathType Container) { $TextureDir = $normal }
}

if ($TextureDir -ne "" -and (Test-Path -LiteralPath $TextureDir -PathType Container)) {
    $blockDest = Join-Path $stage "textures\block"
    [void](Copy-DirectoryContents -Source $TextureDir -Destination $blockDest)
    $blockFiles = @(Get-ChildItem -LiteralPath $blockDest -File -Recurse)
    $bytes = ($blockFiles | Measure-Object -Property Length -Sum).Sum
    if ($null -eq $bytes) { $bytes = 0 }
    Write-Output ("Staging {0} block texture files from {1} ({2:N1} MB)" -f $blockFiles.Count, $TextureDir, ($bytes / 1MB))

    # The published build redistributes the art, so preserve its credit.
    $credit = Join-Path $root "assets\textures\CREDIT.txt"
    if (Test-Path -LiteralPath $credit) {
        Copy-Item -LiteralPath $credit (Join-Path $stage "textures\CREDIT.txt")
        Write-Output ("Crediting: {0}" -f (Get-Content -LiteralPath $credit -TotalCount 1))
    } else {
        Write-Warning "No assets/textures/CREDIT.txt. Check the pack licence before publishing its textures."
    }
} else {
    Write-Output "No block texture pack staged - the web build will use procedural block textures."
}

# Copy every other texture category the renderer can address (items,
# GUI, entities, colormaps, particles, environment, etc). block32 is a
# legacy staging directory and is deliberately not used as the default.
$texturesRoot = Join-Path $root "assets\textures"
if (Test-Path -LiteralPath $texturesRoot -PathType Container) {
    Get-ChildItem -LiteralPath $texturesRoot -Directory -Force |
        Where-Object { $_.Name -notin @("block", "block32") } |
        ForEach-Object {
            $category = $_.Name
            $destination = Join-Path (Join-Path $stage "textures") $category
            if (Copy-DirectoryContents -Source $_.FullName -Destination $destination) {
                $files = @(Get-ChildItem -LiteralPath $destination -File -Recurse)
                Write-Output ("Staging {0} {1} texture files" -f $files.Count, $category)
            }
        }
}

# Player skins are user content, not the vanilla mob sheets. Include the
# selected wide/slim skins and the custom skin when present.
$skinRoot = Join-Path $root "assets\skins"
$skinStage = Join-Path $stage "skins"
$playerSkinCount = 0
foreach ($style in @("wide", "slim")) {
    $source = Join-Path $skinRoot $style
    $destination = Join-Path $skinStage $style
    if (Copy-DirectoryContents -Source $source -Destination $destination) {
        $playerSkinCount += @(Get-ChildItem -LiteralPath $destination -File -Recurse).Count
    }
}
$customSkin = Join-Path $skinRoot "custom.png"
if (Test-Path -LiteralPath $customSkin -PathType Leaf) {
    New-Item -ItemType Directory -Force -Path $skinStage | Out-Null
    Copy-Item -LiteralPath $customSkin -Destination (Join-Path $skinStage "custom.png") -Force
    $playerSkinCount++
}
Write-Output ("Staging {0} player skin files" -f $playerSkinCount)

# Vanilla entity sheets are copyrighted separately from player skins.
# Keep them opt-in for public builds; only pass -MobTextures when the pack
# may legally be redistributed. Its credit file travels with the sheets.
$mobDir = Join-Path $skinRoot "mob"
if ($MobTextures -and (Test-Path -LiteralPath $mobDir -PathType Container)) {
    $mobStage = Join-Path $skinStage "mob"
    [void](Copy-DirectoryContents -Source $mobDir -Destination $mobStage)
    $mobFiles = @(Get-ChildItem -LiteralPath $mobStage -File -Recurse)
    $mobBytes = ($mobFiles | Measure-Object -Property Length -Sum).Sum
    if ($null -eq $mobBytes) { $mobBytes = 0 }
    $shield = Join-Path $skinRoot "shield.png"
    if (Test-Path -LiteralPath $shield -PathType Leaf) {
        Copy-Item -LiteralPath $shield -Destination (Join-Path $skinStage "shield.png") -Force
    }
    Write-Output ("Staging {0} mob texture files ({1:N0} KB)" -f $mobFiles.Count, ($mobBytes / 1KB))
    $mobCredit = Join-Path $mobDir "CREDIT.txt"
    if (Test-Path -LiteralPath $mobCredit) {
        Write-Output ("Mob textures credited: {0}" -f (Get-Content -LiteralPath $mobCredit -TotalCount 1))
    } else {
        Write-Warning "No assets/skins/mob/CREDIT.txt - check the mob pack's redistribution terms."
    }
} elseif (Test-Path -LiteralPath $mobDir -PathType Container) {
    Write-Output "Mob textures left out of the download (pass -MobTextures only when redistribution is permitted)."
}

# Audio is optional because the engine synthesizes replacements. Keep
# third-party/vanilla sound packs opt-in for public builds.
$soundDir = Join-Path $root "assets\sounds"
if ($SoundFiles -and (Test-Path -LiteralPath $soundDir -PathType Container)) {
    $soundStage = Join-Path $stage "sounds"
    [void](Copy-DirectoryContents -Source $soundDir -Destination $soundStage)
    $soundFiles = @(Get-ChildItem -LiteralPath $soundStage -File -Recurse)
    Write-Output ("Staging {0} sound files" -f $soundFiles.Count)
    if (Test-Path -LiteralPath (Join-Path $soundDir "CREDIT.txt")) {
        Copy-Item -LiteralPath (Join-Path $soundDir "CREDIT.txt") -Destination (Join-Path $soundStage "CREDIT.txt") -Force
    } else {
        Write-Warning "No assets/sounds/CREDIT.txt - check the sound pack's redistribution terms."
    }
} elseif (Test-Path -LiteralPath $soundDir -PathType Container) {
    Write-Output "Sound files left out of the download (pass -SoundFiles only when redistribution is permitted)."
}

$stageRoot = (Resolve-Path (Join-Path $root "build-web\stage\assets")).Path.Replace('\','/')

# emcmake needs cmake and ninja on PATH. On Windows they usually come
# bundled with Visual Studio rather than installed standalone.
$extraPaths = @()
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    $vsRoots = @(
        "C:\Program Files\Microsoft Visual Studio\18\Community",
        "C:\Program Files\Microsoft Visual Studio\18\BuildTools",
        "C:\Program Files\Microsoft Visual Studio\2022\Community",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools"
    )
    foreach ($vs in $vsRoots) {
        $cmakeBin = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
        $ninjaBin = Join-Path $vs "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"
        if (Test-Path -LiteralPath $cmakeBin) { $extraPaths += $cmakeBin; $extraPaths += $ninjaBin; break }
    }
    if ($extraPaths.Count -eq 0) {
        Write-Error "cmake not found. Install CMake, or Visual Studio with the C++ workload."
        exit 1
    }
    $env:PATH = ($extraPaths -join ";") + ";" + $env:PATH
}

$cmds = @(
    "call `"$Emsdk\emsdk_env.bat`" >nul 2>&1",
    "emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release -DWEB_ASSET_DIR=`"$stageRoot`"",
    "cmake --build build-web"
) -join " && "

cmd /c $cmds
if ($LASTEXITCODE -ne 0) { Write-Error "Web build failed"; exit 1 }

# Collect just the files that need uploading. build-web/ is full of CMake
# scratch; dist/ is the folder you actually drag onto Netlify.
$dist = Join-Path $root "dist"
New-Item -ItemType Directory -Force -Path $dist | Out-Null
# Clear the contents rather than the folder itself: a shell or web server
# sitting in dist/ holds a lock on the directory but not on its files.
Get-ChildItem $dist -Force | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
foreach ($name in @("index.html", "index.js", "index.wasm", "index.data")) {
    $built = Join-Path $root "build-web\$name"
    if (Test-Path $built) { Copy-Item $built $dist }
}
Copy-Item (Join-Path $root "web\netlify.toml") $dist -ErrorAction SilentlyContinue
Copy-Item (Join-Path $root "web\_headers") $dist -ErrorAction SilentlyContinue
Copy-Item (Join-Path $root "web\_redirects") $dist -ErrorAction SilentlyContinue

# Also leave a zip beside it. Netlify's drag-and-drop takes either a
# folder or a zip, and a single file is easier to hand to someone.
$zip = Join-Path $root "browsercraft-netlify.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $dist "*") -DestinationPath $zip

Write-Output ""
Write-Output "Deploy folder: dist/"
$total = 0
Get-ChildItem $dist | ForEach-Object {
    $total += $_.Length
    Write-Output ("  {0,-16} {1:N2} MB" -f $_.Name, ($_.Length / 1MB))
}
Write-Output ("  {0,-16} {1:N2} MB total" -f "", ($total / 1MB))
Write-Output ""
Write-Output ("Upload either one to Netlify:")
Write-Output ("  dist/                        (drag the folder)")
Write-Output ("  browsercraft-netlify.zip     ({0:N2} MB, drag the file)" -f ((Get-Item $zip).Length / 1MB))

if ($Serve) {
    Write-Output ""
    Write-Output "Serving http://localhost:$Port/index.html  (Ctrl+C to stop)"
    Set-Location (Join-Path $root "build-web")
    python -m http.server $Port
}
