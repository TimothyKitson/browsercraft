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
} else {
    Write-Output "No texture pack staged - the web build will use procedural textures."
}

$stageRoot = (Resolve-Path (Join-Path $root "build-web\stage\assets")).Path.Replace('\','/')

$cmds = @(
    "call `"$Emsdk\emsdk_env.bat`" >nul 2>&1",
    "emcmake cmake -S . -B build-web -G Ninja -DCMAKE_BUILD_TYPE=Release -DWEB_ASSET_DIR=`"$stageRoot`"",
    "cmake --build build-web"
) -join " && "

cmd /c $cmds
if ($LASTEXITCODE -ne 0) { Write-Error "Web build failed"; exit 1 }

Write-Output ""
Write-Output "Built:"
Get-ChildItem build-web -Filter "index.*" | ForEach-Object {
    Write-Output ("  {0,-16} {1:N1} MB" -f $_.Name, ($_.Length / 1MB))
}

if ($Serve) {
    Write-Output ""
    Write-Output "Serving http://localhost:$Port/index.html  (Ctrl+C to stop)"
    Set-Location (Join-Path $root "build-web")
    python -m http.server $Port
}
