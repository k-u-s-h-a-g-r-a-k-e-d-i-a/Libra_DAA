@echo off
cd /d "%~dp0"
call build.bat
if errorlevel 1 (
  pause
  exit /b 1
)
libradaa.exe
pause
