@echo off
rem Firing Range helper for Windows. Forwards everything to Scripts\fr.ps1.
rem Run "fr help" to see the commands.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\fr.ps1" %*
exit /b %ERRORLEVEL%
