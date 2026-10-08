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
    [string]$TextureDir = "",   # defaults to assets/textures/block32 when it exists
    [switch]$MobTextures,       # see below: vanilla entity textures are not yours to publish
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

# Stage the assets that get baked into the download.
$stage = Join-Path $root "build-web\stage\assets"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Copy-Item -Recurse (Join-Path $root "assets\shaders") (Join-Path $stage "shaders")

if ($TextureDir -eq "") {
    $lowRes = Join-Path $root "assets\textures\block32"
    $normal = Join-Path $root "assets\textures\block"
    if (Test-Path $lowRes) { $TextureDir = $lowRes }
    elseif (Test-Path $normal) { $TextureDir = $normal }
}

if ($TextureDir -ne "" -and (Test-Path -LiteralPath $TextureDir)) {
    New-Item -ItemType Directory -Force -Path (Join-Path $stage "textures") | Out-Null
    Copy-Item -Recurse -LiteralPath $TextureDir (Join-Path $stage "textures\block")
    $bytes = (Get-ChildItem (Join-Path $stage "textures\block") -File | Measure-Object -Property Length -Sum).Sum
    Write-Output ("Staging textures from {0} ({1:N1} MB)" -f $TextureDir, ($bytes / 1MB))

    # The published build is the one that actually redistributes the art,
    # so the credit has to go with it or the title screen credits nobody.
    $credit = Join-Path $root "assets\textures\CREDIT.txt"
    if (Test-Path -LiteralPath $credit) {
        Copy-Item -LiteralPath $credit (Join-Path $stage "textures\CREDIT.txt")
        Write-Output ("Crediting: {0}" -f (Get-Content -LiteralPath $credit -TotalCount 1))
    } else {
        Write-Warning "No assets/textures/CREDIT.txt - the published build will credit nobody."
        Write-Warning "Most packs are free to redistribute only if their author is credited."
    }
} else {
    Write-Output "No texture pack staged - the web build will use procedural textures."
}

# Mob sheets are a separate decision from block textures. A block pack is
# usually somebody's own work, passed on under their own terms; the
# vanilla entity textures most people install here are Mojang's, and
# putting them on a public site is republishing Mojang's art. So they
# stay out of the download unless you say otherwise, and the game falls
# back to the hides it paints for itself.
$mobDir = Join-Path $root "assets\skins\mob"
if ($MobTextures -and (Test-Path -LiteralPath $mobDir)) {
    New-Item -ItemType Directory -Force -Path (Join-Path $stage "skins") | Out-Null
    Copy-Item -Recurse -LiteralPath $mobDir (Join-Path $stage "skins\mob")
    $mobBytes = (Get-ChildItem (Join-Path $stage "skins\mob") -File | Measure-Object -Property Length -Sum).Sum
    Write-Output ("Staging mob textures ({0:N0} KB)" -f ($mobBytes / 1KB))
    Write-Warning "Publishing mob textures. Make sure they are yours to pass on - the vanilla ones are not."

    $mobCredit = Join-Path $mobDir "CREDIT.txt"
    if (Test-Path -LiteralPath $mobCredit) {
        Write-Output ("Mob textures credited: {0}" -f (Get-Content -LiteralPath $mobCredit -TotalCount 1))
    } else {
        Write-Warning "No assets/skins/mob/CREDIT.txt - the published mobs will credit nobody."
    }
} elseif (Test-Path -LiteralPath $mobDir) {
    Write-Output "Mob textures left out of the download (pass -MobTextures to include them)."
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
