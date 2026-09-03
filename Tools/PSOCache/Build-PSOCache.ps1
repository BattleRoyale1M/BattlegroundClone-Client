<#
.SYNOPSIS
  Expand recorded PSOs + shader stable keys into a bundled *.stable.upipelinecache
  and drop it in Build/<Platform>/PipelineCaches/ for the next package to bundle.

.DESCRIPTION
  Prerequisites (in order):
    1. Cook once with bShareMaterialShaderCode=True (engine default) -> emits *.scl.csv
       shader stable-key files under Saved/Cooked/<Platform>/.../Metadata/.
    2. Record a play session with Record-PSOs.ps1 -> *.rec.upipelinecache under
       Saved/CollectedPSOs/.
  This script runs the ShaderPipelineCacheTools "Expand" commandlet to merge them.

  After it succeeds, re-run the package build. RunUAT detects the *.stable.upipelinecache,
  builds the binary *.upipelinecache and stages it into the pak.

.PARAMETER Engine
  UE install root. Default: C:\Program Files\Epic Games\UE_5.8
.PARAMETER Platform
  Cooked platform folder name. Default: Windows
.PARAMETER ShaderPlatform
  Shader platform tag used in the output filename. Default: SF_D3D_SM6
#>
param(
    [string]$Engine         = "C:\Program Files\Epic Games\UE_5.8",
    [string]$Platform       = "Windows",
    [string]$ShaderPlatform = "SF_D3D_SM6"
)
$ErrorActionPreference = "Stop"

$root = Resolve-Path "$PSScriptRoot\..\.."
$proj = Join-Path $root "BattlegroundClone.uproject"
$cmd  = Join-Path $Engine "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
if (-not (Test-Path $cmd))  { throw "UnrealEditor-Cmd.exe not found: $cmd  (pass -Engine)" }
if (-not (Test-Path $proj)) { throw "Project not found: $proj" }

# --- 1. recordings -----------------------------------------------------------
$recs = Get-ChildItem (Join-Path $root "Saved\CollectedPSOs") -Recurse -Filter *.rec.upipelinecache -ErrorAction SilentlyContinue
if (-not $recs) { throw "No *.rec.upipelinecache under Saved/CollectedPSOs. Run Record-PSOs.ps1 first." }

# --- 2. shader stable keys from the last cook -------------------------------
$scls = Get-ChildItem (Join-Path $root "Saved\Cooked\$Platform") -Recurse -Filter *.scl.csv -ErrorAction SilentlyContinue
if (-not $scls) {
    throw "No *.scl.csv (shader stable keys) under Saved/Cooked/$Platform. Cook once first " +
          "(bShareMaterialShaderCode is on by default)."
}

# --- 3. expand -------------------------------------------------------------
$outDir = Join-Path $root "Build\$Platform\PipelineCaches"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
$stable = Join-Path $outDir "BattlegroundClone_$ShaderPlatform.stable.upipelinecache"

$argv = @("`"$proj`"", "-run=ShaderPipelineCacheTools", "Expand")
$argv += ($recs.FullName | ForEach-Object { "`"$_`"" })
$argv += ($scls.FullName | ForEach-Object { "`"$_`"" })
$argv += "`"$stable`""

Write-Host "Recordings : $($recs.Count)" -ForegroundColor DarkGray
Write-Host "Stable keys: $($scls.Count)" -ForegroundColor DarkGray
Write-Host "$cmd $($argv -join ' ')`n" -ForegroundColor DarkGray

& $cmd @argv
if ($LASTEXITCODE -ne 0) {
    throw "ShaderPipelineCacheTools Expand failed ($LASTEXITCODE). Run '$cmd `"$proj`" -run=ShaderPipelineCacheTools help' to check arg order for this engine build."
}

Write-Host "`nBundled stable cache:" -ForegroundColor Green
Get-Item $stable | Select-Object LastWriteTime, @{n='SizeKB';e={[math]::Round($_.Length/1KB,1)}}, FullName | Format-Table -AutoSize
Write-Host "Next: re-run the package build. Verify the staged log shows a matching" -ForegroundColor Green
Write-Host "'.upipelinecache' under Content/PipelineCaches/$Platform and, at runtime," -ForegroundColor Green
Write-Host "LogShaderPipelineCache reporting tasks with 0 remaining before gameplay." -ForegroundColor Green
