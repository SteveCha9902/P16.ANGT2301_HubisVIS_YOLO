@echo off
rem Build HubisVIS deploy package - all-in-one folder (double-click to run)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Make-DeployPackage.ps1" -Full %*
pause
