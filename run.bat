@echo off

set PYTHON=

:: 1. Try py launcher (most reliable on Windows)
where py >nul 2>&1
if not errorlevel 1 (
    py -3 --version >nul 2>&1
    if not errorlevel 1 set PYTHON=py -3
)

:: 2. Try python in PATH (skip Windows Store stub)
if "%PYTHON%"=="" (
    for /f "delims=" %%i in ('where python 2^>nul') do (
        if "%PYTHON%"=="" (
            echo %%i | findstr /i "WindowsApps" >nul
            if errorlevel 1 set PYTHON=%%i
        )
    )
)

:: 3. Search common install locations
if "%PYTHON%"=="" (
    for %%v in (313 312 311 310 39 38) do (
        if "%PYTHON%"=="" (
            for %%d in (
                "%LOCALAPPDATA%\Programs\Python\Python%%v\python.exe"
                "%LOCALAPPDATA%\Python\pythoncore-3.%%v-64\python.exe"
                "C:\Python%%v\python.exe"
                "%ProgramFiles%\Python%%v\python.exe"
            ) do (
                if "%PYTHON%"=="" if exist %%d set PYTHON=%%d
            )
        )
    )
)

:: 4. Broader search in common folders
if "%PYTHON%"=="" (
    for /f "delims=" %%f in ('dir /b /s "%LOCALAPPDATA%\python.exe" 2^>nul ^| findstr /v "WindowsApps" ^| head -1') do (
        if "%PYTHON%"=="" set PYTHON=%%f
    )
)

if "%PYTHON%"=="" goto nopython

echo Found Python: %PYTHON%
echo Installing matplotlib...
%PYTHON% -m pip install matplotlib -q

echo Starting monitor...
start "Sensor Simulator" /min cmd /c "timeout /t 3 >nul && sensor_sim\sensor_sim.exe"
%PYTHON% monitor\monitor.py
goto end

:nopython
echo.
echo =====================================================
echo  Python not found!
echo.
echo  1. Download Python 3 from:
echo     https://www.python.org/downloads/
echo.
echo  2. During install, CHECK this box:
echo     [v] Add Python to PATH
echo.
echo  3. Re-run this script after installing.
echo =====================================================
echo.
pause

:end
