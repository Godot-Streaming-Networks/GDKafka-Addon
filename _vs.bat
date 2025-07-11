@echo off

setlocal

:: Shutdown any Visual Studio instances that have Godot-Kafka in their MainWindowTitle
for /f "tokens=2 delims=," %%i in ('tasklist /FI "IMAGENAME eq devenv.exe" /FO CSV /NH') do (
    powershell -Command "$process = Get-Process -Id %%i; if ($process.MainWindowTitle -like '*Godot-Kafka*') { $process.Kill(); Write-Host 'Killed process %%i with MainWindowTitle containing Godot-Kafka.' }"
)

:: if exist .sln (
::     @REM Delete the .sln folder
::     rmdir /s /q .sln
:: )
:: if exist .build (
::     @REM Delete the .build folder
::     rmdir /s /q .build
:: )

mkdir .sln
mkdir ".sln"
pushd ".sln"

cmake -G "Visual Studio 17 2022" ../
if %ERRORLEVEL% neq 0 (
    echo CMake failed.
    pause
    exit /b %ERRORLEVEL%
)

REM Create a shortcut to the .sln file
set "shortcutName=Project.sln"
set "targetPath=%~dp0.sln\Godot-Kafka.sln"
set "shortcutPath=%~dp0%shortcutName%.lnk"

:: Delete the shortcut if it already exists
if exist "%shortcutPath%" (
    del "%shortcutPath%"
)

REM Check if the shortcut already exists
if exist "%shortcutPath%" (
    echo Shortcut already exists.
) else (
    echo Creating shortcut...
    powershell -Command "$WshShell = New-Object -ComObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%shortcutPath%'); $Shortcut.TargetPath = '%targetPath%'; $Shortcut.Save()"
)

start %shortcutPath%

endlocal

exit /b