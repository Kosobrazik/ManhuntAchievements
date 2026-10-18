param(
    # The working copy of the published repository. Defaults to the folder next
    # to the project, which is where it lives after the first push.
    [string]$Path = (Join-Path $PSScriptRoot '../../ManhuntAchievements-public')
)
# Brings an already cloned public repository up to date with this working tree:
# every file git tracks here is copied over, anything no longer tracked is
# removed, and the clone's own .git is left alone. Commit and push from there.
$ErrorActionPreference = 'Stop'
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
. (Join-Path $PSScriptRoot 'source_files.ps1')
if (-not (Test-Path -LiteralPath $Path)) { throw "No such folder: $Path" }
$clone = (Resolve-Path $Path).Path
if ($clone -eq $projectRoot -or
    $clone.StartsWith($projectRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -or
    $projectRoot.StartsWith($clone + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Publish into a separate clone outside the source project.'
}
if (-not (Test-Path -LiteralPath (Join-Path $clone '.git'))) {
    throw "$clone is not a git repository; publish only into a clone of the public repository."
}

$publishable = @(Get-PublishableSourceFiles -ProjectRoot $projectRoot)
$oldFiles = @(& git -C $clone ls-files)
if ($LASTEXITCODE -ne 0) { throw 'Cannot list files in the public clone.' }

foreach ($relative in $publishable) {
    $source = Join-Path $projectRoot $relative
    if (-not (Test-Path -LiteralPath $source)) { throw "Tracked file is missing: $relative" }
    $destination = Join-Path $clone $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

# Remove obsolete tracked files only. Preserve the clone's local build files,
# SDK headers and any other untracked work.
$keep = [System.Collections.Generic.HashSet[string]]::new([string[]]$publishable)
foreach ($relative in $oldFiles) {
    if (-not $keep.Contains($relative)) {
        $obsolete = [IO.Path]::GetFullPath((Join-Path $clone $relative))
        if (-not $obsolete.StartsWith($clone + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw "Unexpected cleanup path: $obsolete"
        }
        if (Test-Path -LiteralPath $obsolete -PathType Leaf) {
            Write-Host "removed: $relative"
            Remove-Item -LiteralPath $obsolete -Force
        }
    }
}

Write-Host "Updated: $clone"
& git -C $clone status --short
