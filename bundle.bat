@echo off
setlocal

rem Set the paths
set OF_PATH=C:\Users\Gondry\Desktop\of_v0.12.0_vs_release
set PROJECT_PATH=C:\Users\Gondry\Desktop\of_v0.12.0_vs_release\apps\myApps\mini-car-of
set DIST_PATH=%PROJECT_PATH%\dist

rem Create dist directory if it doesn't exist
if not exist "%DIST_PATH%" mkdir "%DIST_PATH%"

rem Copy the executable
copy "%PROJECT_PATH%\bin\mini-car-of.exe" "%DIST_PATH%\mini-car-of.exe"

rem Copy necessary DLLs
copy "%OF_PATH%\libs\openFrameworksCompiled\lib\vs\openFrameworksLib.dll" "%DIST_PATH%\"
copy "%OF_PATH%\libs\opencv\lib\vs\opencv_world320.dll" "%DIST_PATH%\"
copy "%OF_PATH%\libs\glew\lib\vs\glew32.dll" "%DIST_PATH%\"

rem Copy resource files
xcopy "%PROJECT_PATH%\bin\data" "%DIST_PATH%\data" /E /I /Y

echo Bundling complete.
endlocal