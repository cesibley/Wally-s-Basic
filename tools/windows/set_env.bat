@echo off
rem Add MSYS2 MinGW bin directories to PATH for this session.
rem Update MSYS2_ROOT if you installed MSYS2 elsewhere.

set "MSYS2_ROOT=C:\msys64"
set "PATH=%MSYS2_ROOT%\ucrt64\bin;%MSYS2_ROOT%\mingw64\bin;%PATH%"

echo MSYS2 PATH configured for this session:
echo   %MSYS2_ROOT%\ucrt64\bin
echo   %MSYS2_ROOT%\mingw64\bin
echo.
echo If you only use one environment, you can remove the other entry.

