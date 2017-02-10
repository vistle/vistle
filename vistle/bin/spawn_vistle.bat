REM @echo off
echo spawn_vistle.bat %1 %2 %3 %4 %5 %6 %7 %8 %9
echo MSMPI_BIN=%MSMPI_BIN%

"%MSMPI_BIN%\mpiexec" %1 %2 %3 %4 %5 %6 %7 %8 %9
