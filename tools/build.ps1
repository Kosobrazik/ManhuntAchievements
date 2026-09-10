param(
    [ValidateSet('Release', 'Debug')][string]$Configuration = 'Release',
    [switch]$Test,
    [switch]$Rebuild
)
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Install Visual Studio C++ build tools and the Windows SDK.' }
$installation = & $vswhere -latest -prerelease -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -format json | ConvertFrom-Json
if (-not $installation) { throw 'Visual Studio C++ x86/x64 tools were not found.' }
$msbuild = Join-Path $installation[0].installationPath 'MSBuild/Current/Bin/MSBuild.exe'
$major = ([version]$installation[0].installationVersion).Major
if ($major -ge 18) { $toolset = 'v145' } elseif ($major -ge 17) { $toolset = 'v143' } else { throw 'Visual Studio 2022 or newer is required.' }
$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
$project = Join-Path $projectRoot 'source/ManhuntAchievements.vcxproj'
& $msbuild $project "/t:$target" "/p:Configuration=$Configuration" /p:Platform=Win32 "/p:PlatformToolset=$toolset" /m /nologo /verbosity:minimal
if ($LASTEXITCODE -ne 0) { throw "Plugin build failed ($LASTEXITCODE)." }
if ($Test) {
    & $msbuild (Join-Path $projectRoot 'tests/PersistenceTests.vcxproj') "/t:$target" /p:Configuration=Release /p:Platform=Win32 "/p:PlatformToolset=$toolset" /m /nologo /verbosity:minimal
    if ($LASTEXITCODE -ne 0) { throw "Test build failed ($LASTEXITCODE)." }
    & (Join-Path $projectRoot 'build/tests/AchievementPersistenceTests.exe') (Join-Path $projectRoot "build/$Configuration/ManhuntAchievements.asi")
    if ($LASTEXITCODE -ne 0) { throw "Persistence tests failed ($LASTEXITCODE)." }
}
