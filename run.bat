@echo off

:: Try py launcher first, then python, then full path
set PYTHON=
where py >nul 2>&1 && set PYTHON=py -3
if "%PYTHON%"=="" (
    python -c "import sys; exit(0 if 'WindowsApps' not in sys.executable else 1)" >nul 2>&1
    if not errorlevel 1 set PYTHON=python
)
if "%PYTHON%"=="" (
    if exist "C:\Users\fhd\AppData\Local\Python\pythoncore-3.14-64\python.exe" (
        set PYTHON=C:\Users\fhd\AppData\Local\Python\pythoncore-3.14-64\python.exe
    )
)
if "%PYTHON%"=="" goto nopython

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
echo  Install from: https://www.python.org/downloads/
echo  Check "Add Python to PATH" during install.
echo =====================================================
pause

:end
