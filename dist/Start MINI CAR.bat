@echo off
setlocal
set "APPDIR=%~dp0app"
set "PATH=%APPDIR%\lib;%PATH%"
cd /d "%APPDIR%"
start "" "mini-car-of.exe"
endlocal
