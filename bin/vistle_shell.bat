@echo off
rem Windows translation of vistle_shell: sets up PATH and PYTHONPATH similar to test.py

rem default host/port
set "HOST=%vistle_config int system net controlport 31093%"
set "PORT=31093"

if not "%1"=="" set "HOST=%1"
if not "%2"=="" set "PORT=%2"

echo HOST=%HOST% PORT=%PORT%

echo ==============================================
echo Vistle shell (Windows)
echo HOST=%HOST% PORT=%PORT%
echo ==============================================

setlocal enabledelayedexpansion
rem find first PATH entry containing \vistle\...\\lib
for /f "usebackq delims=" %%L in (`powershell -NoProfile -Command "($env:PATH -split ';') | ForEach-Object { if ($_ -ne '') { $n = ($_ -replace '/','\\'); if ($n -match '(?i)\\vistle(?:.*)\\lib(?:\\|$)') { Write-Output $n; break } } }"`) do (
    set "VISTLE_LIB=%%~L"
)

rem find first PATH entry containing \vistle\...\\bin
for /f "usebackq delims=" %%B in (`powershell -NoProfile -Command "($env:PATH -split ';') | ForEach-Object { if ($_ -ne '') { $n = ($_ -replace '/','\\'); if ($n -match '(?i)\\vistle(?:.*)\\bin(?:\\|$)') { Write-Output $n; break } } }"`) do (
    set "VISTLE_BIN=%%~B"
)

if defined VISTLE_LIB (
    echo Found VISTLE_LIB from PATH: %VISTLE_LIB%
) else (
    echo Vistle lib dir not found
)

if defined VISTLE_BIN (
    echo Found VISTLE_BIN from PATH: %VISTLE_BIN%
) else (
    echo Vistle bin dir not found
)

rem candidate python / dll dirs
set "C1=%VISTLE_LIB%\..\share\vistle"
set "C2=%VISTLE_LIB%"
set "C3=%VISTLE_LIB%\module"
set "C4=%VISTLE_BIN%"

if not exist "%C1%" (
    echo Vistle share dir not found
)

set PYTHONPATH=%C1%;%C2%;%C3%;%C4%;%PYTHONPATH%
echo PYTHONPATH=%PYTHONPATH%
rem choose python (prefer ipython)
where ipython >nul 2>nul
if errorlevel 1 (
    set "PY=python"
) else (
    set "PY=ipython"
)

rem Run python and connect to Vistle session (interactive) using add_dll_directory
echo Launching %PY% -i -c "import os,sys; os.add_dll_directory(r'%VISTLE_LIB%'); os.add_dll_directory(r'%VISTLE_BIN%'); import vistle; vistle._vistle.sessionConnect(None, '%HOST%', %PORT%); from vistle import *;"
%PY% -i -c "import os,sys; os.add_dll_directory(r'%VISTLE_LIB%'); os.add_dll_directory(r'%VISTLE_BIN%'); import vistle; vistle._vistle.sessionConnect(None, '%HOST%', %PORT%); from vistle import *;" %*

endlocal
