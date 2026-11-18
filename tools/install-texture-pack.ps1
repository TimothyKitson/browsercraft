# Installs the full texture tree from a Minecraft Java resource-pack ZIP.
#
#   powershell -ExecutionPolicy Bypass -File tools\install-texture-pack.ps1 -Zip "C:\path\to\pack.zip" -Credit "Pack author"
#
# Older versions copied a short list of block/item names. Modern packs
# contain hundreds of additional tiles and nested entity/GUI assets, which
# silently fell back to procedural textures. This copies all used texture
# categories plus PNG, animation metadata and PBR texture-set metadata.
param(
    [Parameter(Mandatory = $true)][string]$Zip,
    [string]$Credit = "",
    [switch]$NoPbr
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
if (-not (Test-Path -LiteralPath $Zip -PathType Leaf)) {
    Write-Error "Pack not found: $Zip"
    exit 1
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$archive = [System.IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Zip))

$categoryMap = @{
    "block" = "block"; "blocks" = "block"
    "item" = "item"; "items" = "item"
    "gui" = "gui"; "entity" = "entity"
    "colormap" = "colormap"; "environment" = "environment"
    "particle" = "particle"; "painting" = "painting"
    "mob_effect" = "mob_effect"; "misc" = "misc"
    "map" = "map"; "font" = "font"; "models" = "models"
    "trims" = "trims"; "effect" = "effect"
}

$counts = @{}
$blockFiles = 0
$itemFiles = 0
$pbrFiles = 0
$skipped = 0

try {
    foreach ($entry in $archive.Entries) {
        if ([string]::IsNullOrWhiteSpace($entry.FullName) -or $entry.FullName.EndsWith("/")) { continue }
        $normalized = $entry.FullName.Replace("\", "/")

        # Supports normal resource-pack ZIPs and an extra wrapper folder.
        $match = [regex]::Match($normalized, '(?:^|/)assets/minecraft/textures/([^/]+)/(.+)$',
            [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if (-not $match.Success) {
            $match = [regex]::Match($normalized, '(?:^|/)textures/([^/]+)/(.+)$',
                [System.Text.RegularExpressions.RegexOptions]::IgnoreCase)
        }
        if (-not $match.Success) { continue }

        $sourceCategory = $match.Groups[1].Value.ToLowerInvariant()
        if (-not $categoryMap.ContainsKey($sourceCategory)) { continue }
        $category = $categoryMap[$sourceCategory]
        $relative = $match.Groups[2].Value

        # Never allow a crafted ZIP to write outside the asset directory.
        $segments = $relative -split "[/\\]"
        if ($segments -contains ".." -or [System.IO.Path]::IsPathRooted($relative)) {
            $skipped++
            continue
        }

        $extension = [System.IO.Path]::GetExtension($relative).ToLowerInvariant()
        if ($extension -notin @(".png", ".mcmeta", ".json")) { continue }
        if ($NoPbr -and $relative -match '_(n|s)\.png$') { continue }

        $destination = Join-Path (Join-Path $root "assets\textures\$category") ($relative.Replace("/", "\"))
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination) | Out-Null
        [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $destination, $true)

        if (-not $counts.ContainsKey($category)) { $counts[$category] = 0 }
        $counts[$category]++
        if ($category -eq "block") {
            if ($relative -match '_(n|s)\.png$') { $pbrFiles++ } else { $blockFiles++ }
        }
        if ($category -eq "item" -and $extension -eq ".png") { $itemFiles++ }
    }
}
finally {
    $archive.Dispose()
}

if ($blockFiles -eq 0) {
    Write-Warning "No block PNGs found. Expected assets/minecraft/textures/block/ in the ZIP."
}

# The heart/hotbar renderer still uses flat legacy names; retain those
# while also keeping the complete modern GUI sprite tree.
$guiDir = Join-Path $root "assets\textures\gui"
$guiMap = @{
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
$guiMapped = 0
foreach ($source in $guiMap.Keys) {
    $from = Join-Path $guiDir $source
    if (Test-Path -LiteralPath $from -PathType Leaf) {
        Copy-Item -LiteralPath $from -Destination (Join-Path $guiDir $guiMap[$source]) -Force
        $guiMapped++
    }
}

# Convert mob/player sheets from the extracted entity folder to the
# engine's model-specific assets/skins locations.
$entityDir = Join-Path $root "assets\textures\entity"
if (Test-Path -LiteralPath $entityDir -PathType Container) {
    $mobInstaller = Join-Path $PSScriptRoot "install-mob-textures.ps1"
    if ($Credit -ne "") {
        & $mobInstaller -Folder $entityDir -Credit $Credit
    } else {
        & $mobInstaller -Folder $entityDir
    }
}

$creditPath = Join-Path $root "assets\textures\CREDIT.txt"
if ($Credit -ne "") {
    [System.IO.File]::WriteAllText($creditPath, $Credit,
        (New-Object System.Text.UTF8Encoding($false)))
    Write-Output "Credited as: $Credit"
} elseif (-not (Test-Path -LiteralPath $creditPath)) {
    Write-Warning "No -Credit was supplied. Check the pack's redistribution terms before publishing it."
}

Write-Output "Installed $blockFiles block textures and $pbrFiles PBR maps into assets\textures\block"
Write-Output "Installed $itemFiles item textures into assets\textures\item"
Write-Output "Installed $guiMapped flat GUI sprites (full GUI tree retained)"
foreach ($category in ($counts.Keys | Sort-Object)) {
    Write-Output ("  {0,-12} {1,5} files" -f $category, $counts[$category])
}
if ($skipped -gt 0) { Write-Warning "Skipped $skipped unsafe ZIP paths." }
Write-Output "Rebuild the game or run tools\build-web.ps1 to package the installed textures."
