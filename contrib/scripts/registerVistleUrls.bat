@echo off
@REM This script registers Vistle's url vistle:// to be opened with vistle by placing some registry entries
@REM Vistle needs some environments variables to be set, so copy this somewhere and edit the copy accordingly:

set COVISEDIR=%UserProfile%\Apps\covise
set EXTERNLIBS=%UserProfile%\Apps\externlibs\zebu
set VISTLE_DIR=%UserProfile%\Apps\vistle
set VISTLE_BUILD_DIR=%VISTLE_DIR%\build\zebu
set QT_VERSION=qt6
set ARCHSUFFIX=zebu

@REM Run this script from an administrator cmd

@REM dont touch this

@REM SETLOCAL EnableDelayedExpansion needed to evaluate the if else
SETLOCAL EnableDelayedExpansion 
IF "%1"=="launch" (@REM reuse this script to set the runtime environment for Vistle
    call %COVISEDIR%\winenv.bat %ARCHSUFFIX% %QT_VERSION%
    set PATH=%VISTLE_BUILD_DIR%\bin;%VISTLE_BUILD_DIR%\lib;!PATH!
) else (
    REG ADD HKEY_CLASSES_ROOT\vistle /v "URL Protocol" /f
    REG ADD HKEY_CLASSES_ROOT\vistle\shell /f
    REG ADD HKEY_CLASSES_ROOT\vistle\shell\open /f
    REG ADD HKEY_CLASSES_ROOT\vistle\shell\open\command /f /d "cmd /V /K ""%~dp0registerVistleUrls.bat launch && vistle ""%%1"""""
)
@REM export the local environment
ENDLOCAL & SET "PATH=%PATH%" & set "COVISEDIR=%COVISEDIR%" & set "EXTERNLIBS=%EXTERNLIBS%" & set "PYTHONPATH=%PYTHONPATH%" & set "PYTHONHOME=%PYTHONHOME%"
