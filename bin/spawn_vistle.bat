@echo off
echo spawn_vistle.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
for %%F in ("%1") do set FILENAME=%%~nF
if not defined MPISIZE set MPISIZE=1
if not exist "%TEMP%\vistle" mkdir  "%TEMP%\vistle"
if not "%5"=="" set FILENAME=%FILENAME%-%5.log
@REM start /min /WAIT "mpie" "%MSMPI_BIN%\mpiexec" -n %MPISIZE% %1 %2 %3 %4 %5 %6 %7 %8 %9
"%MSMPI_BIN%\mpiexec" -n %MPISIZE% %1 %2 %3 %4 %5 %6 %7 %8 %9 1> %TEMP%\vistle\%FILENAME% 2>&1
REM start /WAIT "mpi" 
