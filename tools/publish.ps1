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
if (-not (Test-Path -LiteralPath $Path)) { throw "No such folder: $Path" }
$clone = (Resolve-Path $Path).Path
if (-not (Test-Path -LiteralPath (Join-Path $clone '.git'))) {
    throw "$clone is not a git repository; publish only into a clone of the public repository."
}

Push-Location $projectRoot
try {
    $tracked = & git ls-files
    if ($LASTEXITCODE -ne 0) { throw 'git ls-files failed; run this inside the repository.' }
} finally { Pop-Location }
# The RenderWare note lives inside an ignored folder but belongs to the public
# tree, so it is listed explicitly.
$publishable = @($tracked) + @('third_party/rw/README.md') | Sort-Object -Unique

foreach ($relative in $publishable) {
    $source = Join-Path $projectRoot $relative
    if (-not (Test-Path -LiteralPath $source)) { throw "Tracked file is missing: $relative" }
    $destination = Join-Path $clone $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination -Force
}

# Whatever the clone still holds from an earlier version and this tree no longer
# tracks has to go, or the public repository keeps files nobody maintains.
$keep = [System.Collections.Generic.HashSet[string]]::new([string[]]($publishable | ForEach-Object { $_ -replace '/', '\' }))
Get-ChildItem -LiteralPath $clone -Recurse -File -Force |
    Where-Object { $_.FullName -notlike (Join-Path $clone '.git\*') } |
    ForEach-Object {
        $relative = $_.FullName.Substring($clone.Length + 1)
        if (-not $keep.Contains($relative)) {
            Write-Host "removed: $relative"
            Remove-Item -LiteralPath $_.FullName -Force
        }
    }
Get-ChildItem -LiteralPath $clone -Recurse -Directory -Force |
    Where-Object { $_.FullName -notlike (Join-Path $clone '.git\*') -and $_.FullName -ne (Join-Path $clone '.git') } |
    Sort-Object { $_.FullName.Length } -Descending |
    Where-Object { -not (Get-ChildItem -LiteralPath $_.FullName -Force) } |
    Remove-Item -Force

$leaked = Get-ChildItem -LiteralPath $clone -Recurse -File |
    Where-Object { $_.FullName -notlike (Join-Path $clone '.git\*') } |
    Where-Object { $_.Extension -in @('.rpe', '.lua', '.asi', '.log', '.obj', '.pdb') -or $_.Name -eq 'rwcore.h' }
if ($leaked) { throw "Public tree contains files that must not be published: $($leaked.Name -join ', ')" }

Write-Host "Updated: $clone"
& git -C $clone status --short
