# Dev helper: launch the game, move its window to the top-left, let the
# world stream in, grab a screenshot, then shut it down.
param(
    [int]$WaitSeconds = 12,
    [string]$Out = "shot.png",
    [int]$Width = 1100,
    [int]$Height = 700,
    [string]$Keys = ""
)

Set-Location $PSScriptRoot
$exe = Join-Path $PSScriptRoot "build\VoxelEngine.exe"

Add-Type @"
using System;
using System.Runtime.InteropServices;
public class NativeWin {
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int X, int Y, int nWidth, int nHeight, bool bRepaint);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@

$proc = Start-Process -FilePath $exe -ArgumentList @($Width, $Height) -WorkingDirectory (Join-Path $PSScriptRoot "build") `
    -RedirectStandardOutput (Join-Path $PSScriptRoot "game_out.log") `
    -RedirectStandardError (Join-Path $PSScriptRoot "game_err.log") `
    -PassThru

Start-Sleep -Seconds 3
$proc.Refresh()
if ($proc.MainWindowHandle -ne [IntPtr]::Zero) {
    [void][NativeWin]::MoveWindow($proc.MainWindowHandle, 0, 0, $Width, $Height, $true)
    [void][NativeWin]::SetForegroundWindow($proc.MainWindowHandle)
}

Start-Sleep -Seconds $WaitSeconds

if ($proc.HasExited) {
    Write-Output "process exited early with code $($proc.ExitCode)"
    Get-Content (Join-Path $PSScriptRoot "game_err.log") -Tail 20 -ErrorAction SilentlyContinue
    exit 1
}

# Optionally drive the game with a few keystrokes before capturing.
if ($Keys -ne "") {
    [void][NativeWin]::SetForegroundWindow($proc.MainWindowHandle)
    Start-Sleep -Milliseconds 400
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait($Keys)
    Start-Sleep -Milliseconds 900
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bitmap = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
$bitmap.Save((Join-Path $PSScriptRoot $Out), [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bitmap.Dispose()

Stop-Process -Id $proc.Id -Force
Write-Output "captured $Out"
