@echo off
setlocal enabledelayedexpansion
goto :MAIN

@REM function to clone repo
:clone_repo
    setlocal
    set repo_url=%1
    @REM Remove everything before first /
    for /f "tokens=2 delims=/" %%a in (%repo_url%) do set temp=%%a
    @REM Remove everything after first .
    for /f "delims=." %%b in ("%temp%") do set repo_name=%%b
    git clone %repo_url% %TARGET_DIR%\%repo_name%
    endlocal
goto :eof

@REM Start of script
:MAIN

set TARGET_DIR="..\dep_repos"

if not exist %TARGET_DIR% (
    mkdir %TARGET_DIR%
)

for %%r in ("git@github.com:recp/cglm.git"
            "git@github.com:libsdl-org/SDL.git" 
            "git@github.com:nothings/stb.git"
           ) do (
    echo Cloning %%r...
    call :clone_repo %%r
    echo.
)
pause
