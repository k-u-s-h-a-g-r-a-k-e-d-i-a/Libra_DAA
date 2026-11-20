@echo off
cd /d "%~dp0"
gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -D__USE_MINGW_ANSI_STDIO=1 src\main.c src\library.c src\algorithms.c src\verification.c -lm -o libradaa.exe
if errorlevel 1 (
  echo Build failed. Check that GCC is available in PATH.
  pause
  exit /b 1
)
libradaa.exe %*
pause
