@echo off

setlocal EnableDelayedExpansion

REM
REM RefreshEnv.cmd
REM
REM Batch file to read environment variables from registry and
REM set session variables to these values.
REM
REM With this batch file, there should be no need to reload command
REM environment every time you want environment changes to propagate

REM echo "RefreshEnv.cmd only works from cmd.exe, please install the Chocolatey Profile to take advantage of refreshenv from PowerShell"
echo | set /p dummy="Refreshing environment variables from registry for cmd.exe. Please wait..."

goto main

REM Set one environment variable from registry key
:SetFromReg
    "%WinDir%\System32\Reg" QUERY "%~1" /v "%~2" > "%TEMP%\_envset.tmp" 2>NUL
    for /f "usebackq skip=2 tokens=2,*" %%A IN ("%TEMP%\_envset.tmp") do (
        set "tempvar=%%B"
        REM Remove double quotes from temporary value
        set "__tempvar=!tempvar:"=!"

        REM Only escape percentage signs when the value type
        REM is not defined as an expandable string.
        if /I NOT "%%~A"=="REG_EXPAND_SZ" (
            set "tempvar=!tempvar:%%=%%%%!"
        )

        REM If the dequoted string differs from the original string, the variable contains double quotes.
        REM Escape the | and & in the variable to avoid errors.
        if NOT "!__tempvar!" == "!tempvar!" (
            set "tempvar=!tempvar:|=^|!"
            set "tempvar=!tempvar:&=^&!"
        )

        echo/set "%~3=!tempvar!"
    )
    goto :EOF

REM Get a list of environment variables from registry
:GetRegEnv
    "%WinDir%\System32\Reg" QUERY "%~1" > "%TEMP%\_envget.tmp"
    for /f "usebackq skip=2" %%A IN ("%TEMP%\_envget.tmp") do (
        if /I not "%%~A"=="Path" (
            call :SetFromReg "%~1" "%%~A" "%%~A"
        )
    )
    goto :EOF

:main
    echo/@echo off >"%TEMP%\_env.cmd"

    REM Slowly generating final file
    call :GetRegEnv "HKLM\System\CurrentControlSet\Control\Session Manager\Environment" >> "%TEMP%\_env.cmd"
    call :GetRegEnv "HKCU\Environment" >> "%TEMP%\_env.cmd"

    REM Special handling for PATH - mix both User and System
    call :SetFromReg "HKLM\System\CurrentControlSet\Control\Session Manager\Environment" Path Path_HKLM >> "%TEMP%\_env.cmd"
    call :SetFromReg "HKCU\Environment" Path Path_HKCU >> "%TEMP%\_env.cmd"

    endlocal

    REM Caution: do not insert space-chars before >> redirection sign
    echo/set "Path=%%Path_HKLM%%;%%Path_HKCU%%" >> "%TEMP%\_env.cmd"

    REM Cleanup
    del /f /q "%TEMP%\_envset.tmp" 2>nul
    del /f /q "%TEMP%\_envget.tmp" 2>nul

    REM capture user / architecture / volatile variables
    SET "OriginalUserName=%USERNAME%"
    SET "OriginalArchitecture=%PROCESSOR_ARCHITECTURE%"
    SET "OriginalAppData=%APPDATA%"
    SET "OriginalLocalAppData=%LOCALAPPDATA%"
    SET "OriginalUserProfile=%USERPROFILE%"
    SET "OriginalHomeDrive=%HOMEDRIVE%"
    SET "OriginalHomePath=%HOMEPATH%"

    REM Set these variables
    call "%TEMP%\_env.cmd"

    REM Cleanup
    del /f /q "%TEMP%\_env.cmd" 2>nul

    REM reset user / architecture / volatile variables
    SET "USERNAME=%OriginalUserName%"
    SET "PROCESSOR_ARCHITECTURE=%OriginalArchitecture%"
    SET "APPDATA=%OriginalAppData%"
    SET "LOCALAPPDATA=%OriginalLocalAppData%"
    SET "USERPROFILE=%OriginalUserProfile%"
    SET "HOMEDRIVE=%OriginalHomeDrive%"
    SET "HOMEPATH=%OriginalHomePath%"

    echo | set /p dummy="Finished."
    echo .