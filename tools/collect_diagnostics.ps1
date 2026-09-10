param(
    [Parameter(Mandatory = $true)][string]$GameDirectory,
    [string]$OutputDirectory,
    [ValidateRange(1, 168)][int]$Hours = 24
)
$ErrorActionPreference = 'Stop'
$game = (Resolve-Path -LiteralPath $GameDirectory).Path
if (-not (Test-Path -LiteralPath (Join-Path $game 'manhunt.exe'))) { throw 'manhunt.exe was not found in GameDirectory.' }
if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot '../diagnostics' }
$output = [IO.Path]::GetFullPath($OutputDirectory)
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
$report = Join-Path $output "Manhunt-diagnostics-$stamp"
New-Item -ItemType Directory -Path $report -Force | Out-Null
$notes = [Collections.Generic.List[string]]::new()
$notes.Add("Collected: $(Get-Date -Format o)")
$notes.Add("Game directory: $game")
$notes.Add('Read-only game inspection: no plugins/settings were changed and no game was launched.')

$folders = @($game, (Join-Path $game 'scripts'), (Join-Path $game 'plugins'))
$inventory = foreach ($folder in $folders) {
    if (-not (Test-Path -LiteralPath $folder)) { continue }
    foreach ($file in Get-ChildItem -LiteralPath $folder -File) {
        if ($file.Extension -notin @('.asi', '.dll', '.exe', '.ini', '.log', '.disabled', '.backup')) { continue }
        $hash = if ($file.Extension -in @('.asi', '.dll', '.exe')) {
            (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        } else { '' }
        [pscustomobject]@{ Path = $file.FullName; Bytes = $file.Length; Modified = $file.LastWriteTime.ToString('o'); SHA256 = $hash }
    }
}
$inventory | Export-Csv -LiteralPath (Join-Path $report 'files.csv') -NoTypeInformation -Encoding UTF8

foreach ($folder in $folders) {
    if (-not (Test-Path -LiteralPath $folder)) { continue }
    $label = if ($folder -eq $game) { 'root' } else { Split-Path $folder -Leaf }
    $destination = Join-Path $report $label
    New-Item -ItemType Directory -Path $destination -Force | Out-Null
    foreach ($file in Get-ChildItem -LiteralPath $folder -File) {
        if ($file.Name -match '^(ManhuntAchievements|ManhuntGInput|MHMic|DiscordPlugin|PluginMH|MHP).*\.(log|ini)$') {
            Copy-Item -LiteralPath $file.FullName -Destination $destination
        }
    }
}

try {
    $events = @(Get-WinEvent -FilterHashtable @{
        LogName = 'Application'; Id = 1000, 1001; StartTime = (Get-Date).AddHours(-$Hours)
    } -ErrorAction Stop | Where-Object { $_.Message -match '(?i)manhunt\.exe' } | Select-Object -First 20)
    $events | Select-Object TimeCreated, Id, Message | Format-List | Out-String -Width 300 |
        Set-Content -LiteralPath (Join-Path $report 'windows-crashes.txt') -Encoding UTF8
    foreach ($event in $events) {
        $event.ToXml() | Set-Content -LiteralPath (Join-Path $report "event-$($event.RecordId).xml") -Encoding UTF8
        if ($event.Id -eq 1000) {
            $xml = [xml]$event.ToXml()
            $data = @{}
            $items = @($xml.SelectNodes("//*[local-name()='EventData']/*[local-name()='Data']"))
            foreach ($item in $items) {
                $key = $item.GetAttribute('Name')
                if ($key) { $data[$key] = $item.InnerText }
            }
            # Classic Application Error event 1000 uses unnamed, positional fields.
            if (-not $data.ContainsKey('ModuleName') -and $items.Count -ge 8) {
                $data.ModuleName = $items[3].InnerText
                $data.ExceptionCode = $items[6].InnerText
                $data.FaultingOffset = $items[7].InnerText
            }
            $notes.Add("Crash $($event.TimeCreated): module=$($data.ModuleName), code=$($data.ExceptionCode), offset=$($data.FaultingOffset)")
        }
    }
    if (-not $events.Count) { $notes.Add('No matching Windows crash events in the selected period.') }
} catch {
    $notes.Add("Windows event query: $($_.Exception.Message)")
}
$notes | Set-Content -LiteralPath (Join-Path $report 'SUMMARY.txt') -Encoding UTF8
$archive = "$report.zip"
Compress-Archive -Path (Join-Path $report '*') -DestinationPath $archive
$notes | ForEach-Object { Write-Output $_ }
Write-Output "Diagnostic archive: $archive"
