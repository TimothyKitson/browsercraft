# Dev helper: launch the game, move its window to the top-left, wait for the
# world to stream in, grab a screenshot, then shut it down.
#   powershell -ExecutionPolicy Bypass -File tools\screenshot.ps1 -Out shot.png
param(
    [int]$WaitSeconds = 12,
    [string]$Out = "shot.png",
    [int]$Width = 1100,
    [int]$Height = 700
)

$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class NativeWin {
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@

$proc = Start-Process -FilePath (Join-Path $root "build\VoxelEngine.exe") `
    -ArgumentList @($Width, $Height) -WorkingDirectory (Join-Path $root "build") `
    -RedirectStandardOutput (Join-Path $root "game_out.log") `
    -RedirectStandardError (Join-Path $root "game_err.log") -PassThru

Start-Sleep -Seconds 3
$proc.Refresh()
if ($proc.MainWindowHandle -ne [IntPtr]::Zero) {
    [void][NativeWin]::MoveWindow($proc.MainWindowHandle, 0, 0, $Width, $Height, $true)
    [void][NativeWin]::SetForegroundWindow($proc.MainWindowHandle)
}

Start-Sleep -Seconds $WaitSeconds

if ($proc.HasExited) {
    Write-Output "process exited early with code $($proc.ExitCode)"
    Get-Content (Join-Path $root "game_err.log") -Tail 20 -ErrorAction SilentlyContinue
    exit 1
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

# Capture only the game window's rectangle, not the rest of the desktop.
$screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$w = [Math]::Min($Width, $screen.Width)
$h = [Math]::Min($Height, $screen.Height)
$bitmap = New-Object System.Drawing.Bitmap $w, $h
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.CopyFromScreen((New-Object System.Drawing.Point 0, 0), [System.Drawing.Point]::Empty, (New-Object System.Drawing.Size $w, $h))
$bitmap.Save((Join-Path $root $Out), [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose(); $bitmap.Dispose()

[void]$proc.CloseMainWindow()
if (-not $proc.WaitForExit(10000)) { Stop-Process -Id $proc.Id -Force }
Write-Output "captured $Out"
