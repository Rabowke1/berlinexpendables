# Reicht einen ComfyUI-Workflow (API-Format) ein, wartet auf das Ergebnis und lädt die Ausgabebilder.
# Aufruf: run_comfy.ps1 -Workflow <json> -OutDir <ordner> [-TimeoutSec 600]
param(
    [Parameter(Mandatory = $true)][string]$Workflow,
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int]$TimeoutSec = 600,
    [string]$Server = "http://127.0.0.1:8188"
)
$ErrorActionPreference = "Stop"
$wf = Get-Content -Path $Workflow -Raw -Encoding UTF8
$body = "{`"prompt`": $wf, `"client_id`": `"berlin-expendables`"}"
$resp = Invoke-RestMethod -Uri "$Server/prompt" -Method Post -Body ([System.Text.Encoding]::UTF8.GetBytes($body)) -ContentType "application/json; charset=utf-8" -TimeoutSec 60
if ($resp.node_errors -and ($resp.node_errors.PSObject.Properties.Count -gt 0)) { Write-Output ("NODE-FEHLER: " + ($resp.node_errors | ConvertTo-Json -Depth 6)); exit 1 }
$pid_ = $resp.prompt_id
Write-Output "prompt_id: $pid_"
$sw = [Diagnostics.Stopwatch]::StartNew()
$hist = $null
while ($sw.Elapsed.TotalSeconds -lt $TimeoutSec) {
    Start-Sleep -Seconds 3
    $h = Invoke-RestMethod -Uri "$Server/history/$pid_" -TimeoutSec 30
    $entry = $h.$pid_
    if ($entry) {
        if ($entry.status -and $entry.status.status_str -eq "error") {
            Write-Output "FEHLER im Job:"; Write-Output ($entry.status.messages | ConvertTo-Json -Depth 8); exit 1
        }
        if ($entry.outputs) { $hist = $entry; break }
    }
}
if (-not $hist) { Write-Output "Timeout nach $TimeoutSec s"; exit 1 }
Write-Output ("Dauer: {0:N0} s" -f $sw.Elapsed.TotalSeconds)
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
foreach ($nodeId in $hist.outputs.PSObject.Properties.Name) {
    $imgs = $hist.outputs.$nodeId.images
    foreach ($img in $imgs) {
        $q = "filename=$([uri]::EscapeDataString($img.filename))&subfolder=$([uri]::EscapeDataString($img.subfolder))&type=$($img.type)"
        $dest = Join-Path $OutDir $img.filename
        Invoke-WebRequest -Uri "$Server/view?$q" -OutFile $dest -TimeoutSec 120
        Write-Output "gespeichert: $dest"
    }
}
