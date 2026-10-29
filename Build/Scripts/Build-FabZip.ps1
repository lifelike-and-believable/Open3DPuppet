param(
  [string]$UEPath,
  # The Fab source zip made by Build/Scripts/fab-package.py, or a folder that contains it.
  [string]$Zip,
  # Scratch folder: the zip is extracted to <WorkDir>\Stage and packaged to <WorkDir>\Package.
  # Keep it short: BuildPlugin's intermediate paths under it must stay within 260 characters.
  [string]$WorkDir
)

# Runs RunUAT BuildPlugin on the contents of the Fab source zip, the way Fab's build farm
# would, then checks the resulting binaries. Ported from Open3DBroadcast without its o3ds
# core and WebRTC checks. Nothing is added to the extracted zip, so this also shows that the
# shipped modules build without the excluded test module.
#
# Exit codes: 0 built and checked; 1 BuildPlugin failed, a warning was reported in plugin
# sources, or a check failed; 2 bad arguments.

function Write-Failure([string]$Message) { Write-Host "::error::$Message" }

$UEPath = $UEPath.Trim('"', "'")
$Zip = $Zip.Trim('"', "'")
$WorkDir = $WorkDir.Trim('"', "'")

if (Test-Path -LiteralPath $Zip -PathType Container) {
  $found = @(Get-ChildItem -LiteralPath $Zip -Filter *.zip -File)
  if ($found.Count -ne 1) {
    Write-Failure "Expected exactly one .zip in $Zip, found $($found.Count)."
    exit 2
  }
  $Zip = $found[0].FullName
}
if (!(Test-Path -LiteralPath $Zip -PathType Leaf)) {
  Write-Failure "Fab zip not found: $Zip"
  exit 2
}

$stage = Join-Path $WorkDir "Stage"
$package = Join-Path $WorkDir "Package"
foreach ($d in @($stage, $package)) {
  if (Test-Path -LiteralPath $d) { Remove-Item -LiteralPath $d -Recurse -Force }
}
New-Item -ItemType Directory -Force -Path $stage | Out-Null
Expand-Archive -LiteralPath $Zip -DestinationPath $stage

$descriptors = @(Get-ChildItem -LiteralPath $stage -Recurse -Depth 1 -Filter *.uplugin -File)
if ($descriptors.Count -ne 1) {
  Write-Failure "Expected one .uplugin in the zip's top-level folder, found $($descriptors.Count)."
  exit 1
}
$uplugin = $descriptors[0].FullName
$pluginDir = $descriptors[0].DirectoryName
Write-Host "Extracted $Zip -> $pluginDir"

# --- BuildPlugin, failing on warnings in plugin sources (rule FAB-004) ---
& (Join-Path $PSScriptRoot "Build-Plugin.ps1") `
  -UEPath $UEPath `
  -PluginUPluginPath $uplugin `
  -OutDir $package `
  -TargetPlatforms @("Win64") `
  -Configuration "Development" `
  -FailOnWarnings
if ($LASTEXITCODE -ne 0) {
  Write-Failure "BuildPlugin failed on the Fab zip's contents (exit code $LASTEXITCODE)."
  exit 1
}

# --- Check the binaries BuildPlugin produced ---
$problems = @()
$modules = @((Get-Content -LiteralPath $uplugin -Raw | ConvertFrom-Json).Modules | ForEach-Object { $_.Name })
$binDir = Join-Path $package "Binaries\Win64"
foreach ($m in $modules) {
  $dll = Join-Path $binDir "UnrealEditor-$m.dll"
  if (!(Test-Path -LiteralPath $dll)) {
    $problems += "BuildPlugin produced no editor binary for module $m ($dll)."
  }
}
$excludeList = Join-Path $PSScriptRoot "..\Fab\exclude-modules.txt"
if (Test-Path -LiteralPath $excludeList) {
  $excluded = @(Get-Content -LiteralPath $excludeList | ForEach-Object { ($_ -split '#')[0].Trim() } | Where-Object { $_ })
  foreach ($m in $excluded) {
    $leaked = @(Get-ChildItem -LiteralPath $package -Recurse -File -Filter "*$m*" -ErrorAction SilentlyContinue)
    if ($leaked.Count -gt 0) {
      $problems += "Excluded module $m has files in the BuildPlugin output: $(($leaked | Select-Object -First 3 | ForEach-Object { $_.Name }) -join ', ')"
    }
  }
}

if ($problems.Count -gt 0) {
  foreach ($p in $problems) { Write-Failure $p }
  exit 1
}
Write-Host "[OK] The Fab zip builds with BuildPlugin and produced editor binaries for: $($modules -join ', ')"
exit 0
