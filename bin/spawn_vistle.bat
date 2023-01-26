REM @echo off
echo spawn_vistle.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
echo MSMPI_BIN=%MSMPI_BIN%

if not defined MPISIZE set MPISIZE=1

start /min /WAIT "mpie" "%MSMPI_BIN%\mpiexec" -n %MPISIZE% %1 %2 %3 %4 %5 %6 %7 %8 %9
REM start /WAIT "mpi" 
