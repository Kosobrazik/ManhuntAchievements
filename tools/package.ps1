param(
    [string]$Version = '2.1',
    # Repository URL printed in the player readme; omitted when empty.
    [string]$Repository = ''
)
$ErrorActionPreference = 'Stop'
if ($Version -notmatch '^[a-zA-Z0-9._-]+$') { throw 'Version must be a filename-safe identifier.' }
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'source_files.ps1')
$publishable = @(Get-PublishableSourceFiles -ProjectRoot $projectRoot)
$binary = Join-Path $projectRoot 'build/Release/ManhuntAchievements.asi'
if (-not (Test-Path -LiteralPath $binary)) { throw 'Build Release first with tools/build.ps1.' }
$release = Join-Path $projectRoot "dist/$Version"
if (Test-Path -LiteralPath $release) { throw "dist/$Version already exists; remove it explicitly or choose another version." }
# A published clone must never end up under dist, where the next packaging run
# would wipe it. tools/publish.ps1 updates such a clone in place instead.
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'dist') -Directory -Recurse -Force -Filter '.git' -ErrorAction SilentlyContinue |
    ForEach-Object { throw "A git repository lives under dist ($($_.Parent.FullName)); move it out before packaging." }

# Two destinations, kept apart on purpose. Mod sites get the plugin and one
# plain text file a player can open in Notepad; GitHub gets the repository with
# its markdown and sources.
$modSites = Join-Path $release 'mod-sites'
$github = Join-Path $release 'github'

$staging = Join-Path $release 'ManhuntAchievements'
New-Item -ItemType Directory -Path (Join-Path $staging 'data/txd') -Force | Out-Null
Copy-Item -LiteralPath $binary -Destination $staging
Copy-Item -LiteralPath (Join-Path $projectRoot 'ManhuntAchievements.ini') -Destination $staging
Copy-Item -LiteralPath (Join-Path $projectRoot 'data/txd/achievements.txd') -Destination (Join-Path $staging 'data/txd')
# Translations go in data/ManhuntAchievements/lang; the template ships there as
# .example, which the mod does not read.
New-Item -ItemType Directory -Path (Join-Path $staging 'data/ManhuntAchievements/lang') -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $projectRoot 'data/ManhuntAchievements/lang/template.txt.example') -Destination (Join-Path $staging 'data/ManhuntAchievements/lang')
$readme = Get-Content -LiteralPath (Join-Path $projectRoot 'packaging/README_RU_ENG.txt') -Raw
$readme = $readme -replace '\{REPOSITORY\}\r?\n', $(if ($Repository) { "`r`n  Исходный код / source code: $Repository`r`n" } else { '' })
$readme = $readme.Replace('{VERSION}', $Version)
New-Item -ItemType Directory -Path $modSites -Force | Out-Null
$readme = $readme.Replace("`r`n", "`n").Replace("`n", "`r`n")
[IO.File]::WriteAllText((Join-Path $staging 'README_RU_ENG.txt'), $readme, [Text.UTF8Encoding]::new($true))
Compress-Archive -Path (Join-Path $staging '*') -DestinationPath (Join-Path $modSites "ManhuntAchievements-$Version.zip")
$resolvedStaging = (Resolve-Path -LiteralPath $staging).Path
$expectedStaging = [System.IO.Path]::GetFullPath((Join-Path $projectRoot "dist/$Version/ManhuntAchievements"))
if ($resolvedStaging -ne $expectedStaging -or
    -not $resolvedStaging.StartsWith($projectRoot + [System.IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Unexpected staging cleanup path.'
}
Remove-Item -LiteralPath $staging -Recurse -Force

# Include new non-ignored source files as well as tracked working copies, plus
# the note that stands in for the ignored RenderWare headers.
foreach ($relative in $publishable) {
    $destination = Join-Path $github $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $projectRoot $relative) -Destination $destination
}
$leaked = Get-ChildItem -LiteralPath $github -Recurse -File |
    Where-Object { $_.Extension -in @('.rpe', '.lua', '.asi', '.log', '.obj', '.pdb') -or $_.Name -eq 'rwcore.h' }
if ($leaked) { throw "Public tree contains files that must not be published: $($leaked.Name -join ', ')" }
# Mod sites that ask for sources take the same tree as one archive.
Compress-Archive -Path (Join-Path $github '*') -DestinationPath (Join-Path $modSites "ManhuntAchievements-$Version-source.zip")

Write-Host "mod-sites: $modSites"
Write-Host "github:    $github"
Get-FileHash -LiteralPath $binary -Algorithm SHA256 | Format-List Algorithm, Hash
