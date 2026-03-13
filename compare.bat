@echo off
REM File: compare.bat

setlocal

REM Set these three values and you're all done.
set "PROJECT_DIR=C:\dev\crude"
set "CRUDE_REPO=C:\CRUDE"
set "WINMERGE_EXE=C:\Program Files\WinMerge\WinMergeU.exe"

REM Changes beyond this point are unlikely needed.

for %%I in ("%PROJECT_DIR%") do set "PROJECT_NAME=%%~nxI"
set "REVISIONS_DIR=%CRUDE_REPO%\%PROJECT_NAME%\revisions"

if not exist "%PROJECT_DIR%" (
    echo ERROR: Project folder not found:
    echo %PROJECT_DIR%
    exit /b 1
)

if not exist "%REVISIONS_DIR%" (
    echo ERROR: Revisions folder not found:
    echo %REVISIONS_DIR%
    exit /b 1
)

if not exist "%WINMERGE_EXE%" (
    echo ERROR: WinMerge not found:
    echo %WINMERGE_EXE%
    exit /b 1
)

set "LATEST_REV="

for /f "delims=" %%D in ('dir "%REVISIONS_DIR%" /b /ad /o:-n 2^>nul') do (
    set "LATEST_REV=%%D"
    goto found_latest
)

:found_latest
if not defined LATEST_REV (
    echo ERROR: No revisions found in:
    echo %REVISIONS_DIR%
    exit /b 1
)

set "LATEST_DIR=%REVISIONS_DIR%\%LATEST_REV%"

echo Comparing:
echo   CURRENT: %PROJECT_DIR%
echo   REVISION: %LATEST_DIR%

start "" "%WINMERGE_EXE%" "%PROJECT_DIR%" "%LATEST_DIR%"

exit /b 0