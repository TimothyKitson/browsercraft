param(
    [string]$Zip = "",
    [string]$Folder = ""
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

if (-not $Zip -and -not $Folder) {
    Write-Error "Pass -Zip <textures zip> or -Folder <assets\minecraft\textures folder>"
    exit 1
}

$temp = ""
$source = $Folder
if ($Zip) {
    if (-not (Test-Path -LiteralPath $Zip)) { Write-Error "Archive not found: $Zip"; exit 1 }
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $temp = Join-Path ([System.IO.Path]::GetTempPath()) ("browsercraft-textures-" + [guid]::NewGuid())
    New-Item -ItemType Directory -Force -Path $temp | Out-Null
    [System.IO.Compression.ZipFile]::ExtractToDirectory((Resolve-Path -LiteralPath $Zip), $temp)
    $source = $temp
}

$textures = Get-ChildItem -LiteralPath $source -Recurse -Directory |
    Where-Object { (Test-Path (Join-Path $_.FullName "block")) -and (Test-Path (Join-Path $_.FullName "item")) } |
    Select-Object -First 1
if (-not $textures) {
    if ((Test-Path (Join-Path $source "block")) -and (Test-Path (Join-Path $source "item"))) {
        $textures = Get-Item -LiteralPath $source
    } else {
        Write-Error "No folder with block\ and item\ in it under $source"
        exit 1
    }
}
$src = $textures.FullName

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
foreach ($dir in @("textures\block", "textures\item", "textures\gui")) {
    $path = Join-Path $root "assets\$dir"
    if ((Test-Path $path) -and (Get-ChildItem $path -Force | Select-Object -First 1)) {
        Rename-Item -LiteralPath $path -NewName ((Split-Path $path -Leaf) + ".$stamp")
        Write-Output "Kept the old $dir as $dir.$stamp"
    }
}

$texturesDest = Join-Path $root "assets\textures"
foreach ($dir in @("block", "item")) {
    $dest = Join-Path $texturesDest $dir
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Copy-Item -Path (Join-Path $src "$dir\*.png") -Destination $dest -Force
}

$guiDest = Join-Path $texturesDest "gui"
New-Item -ItemType Directory -Force -Path $guiDest | Out-Null
if (Test-Path (Join-Path $src "gui")) {
    Copy-Item -Path (Join-Path $src "gui\*") -Destination $guiDest -Recurse -Force
}

$flat = [ordered]@{
    "sprites\hud\heart\full.png"               = "heart_full.png"
    "sprites\hud\heart\half.png"               = "heart_half.png"
    "sprites\hud\heart\container.png"          = "heart_container.png"
    "sprites\hud\heart\hardcore_full.png"      = "heart_hardcore_full.png"
    "sprites\hud\heart\hardcore_half.png"      = "heart_hardcore_half.png"
    "sprites\hud\heart\container_hardcore.png" = "heart_hardcore_container.png"
    "sprites\hud\food_full.png"                = "food_full.png"
    "sprites\hud\food_half.png"                = "food_half.png"
    "sprites\hud\food_empty.png"               = "food_empty.png"
    "sprites\hud\armor_full.png"               = "armor_full.png"
    "sprites\hud\armor_half.png"               = "armor_half.png"
    "sprites\hud\armor_empty.png"              = "armor_empty.png"
    "sprites\hud\hotbar.png"                   = "hotbar.png"
    "sprites\hud\hotbar_selection.png"         = "hotbar_selection.png"
}
foreach ($key in $flat.Keys) {
    $from = Join-Path $guiDest $key
    if (Test-Path $from) { Copy-Item -LiteralPath $from -Destination (Join-Path $guiDest $flat[$key]) -Force }
}

foreach ($extra in @("environment", "particle", "painting", "mob_effect", "misc", "map", "font", "colormap",
                     "models", "trims", "effect", "entity")) {
    $from = Join-Path $src $extra
    if (-not (Test-Path $from)) { continue }
    $dest = Join-Path $texturesDest $extra
    if (Test-Path $dest) { Remove-Item -LiteralPath $dest -Recurse -Force }
    Copy-Item -LiteralPath $from -Destination $dest -Recurse -Force
}

$entity = Join-Path $src "entity"
if (Test-Path $entity) {
    & (Join-Path $PSScriptRoot "install-mob-textures.ps1") -Folder $entity -Credit "Minecraft 1.21 (Mojang Studios) -- not for redistribution"
}

Set-Content -LiteralPath (Join-Path $texturesDest "CREDIT.txt") -Value "Minecraft 1.21 (Mojang Studios) -- not for redistribution" -Encoding ASCII

if ($temp) { Remove-Item -LiteralPath $temp -Recurse -Force }

$blocks = (Get-ChildItem (Join-Path $texturesDest "block") -Filter *.png).Count
$items = (Get-ChildItem (Join-Path $texturesDest "item") -Filter *.png).Count
$gui = (Get-ChildItem $guiDest -Recurse -Filter *.png).Count
Write-Output "Installed $blocks block textures, $items items and $gui GUI files into assets\textures"
