function Get-PublishableSourceFiles {
    param([Parameter(Mandatory = $true)][string]$ProjectRoot)

    Push-Location $ProjectRoot
    try {
        $files = @(& git ls-files --cached --others --exclude-standard -- .)
        if ($LASTEXITCODE -ne 0) { throw 'git ls-files failed; run inside the repository.' }
        # Deleted tracked files must disappear from exports. New source files must
        # be included even before the working tree has been committed.
        $files = @($files | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf })
        $files = @($files) + @('third_party/rw/README.md') | Sort-Object -Unique
        foreach ($relative in $files) {
            if ($relative -match '(^|/)(build|dist|diagnostics|mh_achievements|\.git)/' -or
                [IO.Path]::GetExtension($relative) -in @('.rpe', '.lua', '.asi', '.zip', '.log', '.obj', '.pdb') -or
                ($relative -like 'third_party/rw/*' -and $relative -ne 'third_party/rw/README.md')) {
                throw "File is not publishable: $relative"
            }
            if (-not (Test-Path -LiteralPath $relative -PathType Leaf)) {
                throw "Source file is missing: $relative"
            }
            $relative
        }
    } finally { Pop-Location }
}
