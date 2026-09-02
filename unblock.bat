@echo off
set /p "folderPath=Paste the folder path: "

if not exist "%folderPath%" (
    echo Folder not found.
    pause
    exit /b 1
)

echo.
echo 1. Unblock all files recursively (includes subfolders)
echo 2. Unblock files in that folder only
echo.
set /p "choice=Select option (1 or 2): "

if "%choice%"=="1" (
    powershell -Command "Get-ChildItem -Path '%folderPath%' -Recurse | Unblock-File"
) else if "%choice%"=="2" (
    powershell -Command "Get-ChildItem -Path '%folderPath%' | Unblock-File"
) else (
    echo Invalid option.
    pause
    exit /b 1
)

echo.
echo Done.
pause
