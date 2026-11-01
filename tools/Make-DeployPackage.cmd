@echo off
rem Build HubisVIS deploy package (double-click to run)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Make-DeployPackage.ps1" %*
pause
