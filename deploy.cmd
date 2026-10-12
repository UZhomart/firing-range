@echo off
rem Double-click this file to bring the project from a USB drive onto this
rem computer: it copies the project to the desktop, installs what is missing
rem and starts the game. Same as "fr deploy".
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\fr.ps1" deploy %*
echo.
pause
