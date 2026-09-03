<#
.SYNOPSIS
  Launch the packaged game with PSO logging on, so a play session records the
  pipeline states the game actually uses.

.DESCRIPTION
  Run this against a cooked/packaged Development (or Test) build - NOT the editor.
  While it runs, exercise EVERYTHING that draws: every map, every weapon/scope,
  every vehicle, every VFX/decal, every menu and HUD state, day and night.
  Missed content = a hitch left in the shipping build.

  Quit through the in-game menu (a clean shutdown). Alt-F4 / crash may lose the
  recording. Output lands in:
    Saved/CollectedPSOs/PipelineCaches/<Platform>/*.rec.upipelinecache

  Then run Build-PSOCache.ps1 to turn the recordings into a bundled cache.

.PARAMETER Exe
  Path to the packaged executable. Defaults to the repo's Package/ output.
#>
param(
    [string]$Exe = "$PSScriptRoot\..\..\Package\Windows\BattlegroundClone.exe"
)
$ErrorActionPreference = "Stop"

if (-not (Test-Path $Exe)) {
    throw "Packaged exe not found: $Exe`nCook/package first (see docs/2026-09-03-bundled-pso-cache-setup.md)."
}

$root = Resolve-Path "$PSScriptRoot\..\.."
$collected = Join-Path $root "Saved\CollectedPSOs"

Write-Host "Launching with -logPSO." -ForegroundColor Cyan
Write-Host "Play through ALL content, then quit via the in-game menu (not Alt-F4)." -ForegroundColor Cyan

& $Exe -logPSO -messaging
$code = $LASTEXITCODE

Write-Host "`nGame exited ($code). Recordings found:" -ForegroundColor Cyan
$recs = Get-ChildItem $collected -Recurse -Filter *.rec.upipelinecache -ErrorAction SilentlyContinue
if ($recs) {
    $recs | Select-Object LastWriteTime, @{n='SizeKB';e={[math]::Round($_.Length/1KB,1)}}, FullName | Format-Table -AutoSize
    Write-Host "Next: Tools/PSOCache/Build-PSOCache.ps1" -ForegroundColor Green
} else {
    Write-Warning "No *.rec.upipelinecache under $collected."
    Write-Warning "Check the build was cooked (not editor) and you quit cleanly."
}
