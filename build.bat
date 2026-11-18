@echo off
cd /d "%~dp0"
where gcc >nul 2>nul
if errorlevel 1 (
  echo GCC C compiler was not found in PATH. Use the MinGW folder that already provided g++.
  exit /b 1
)
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -D__USE_MINGW_ANSI_STDIO=1 src\main.c src\library.c src\algorithms.c src\workbench.c src\evidence.c src\tests.c -lm -o libradaa.exe
if errorlevel 1 exit /b 1
libradaa.exe --test
