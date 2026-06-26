@echo off

:: Check Python
where python >nul 2>&1
if errorlevel 1 goto nopython

python -c "import sys; exit(0 if sys.version_info.major>=3 else 1)" >nul 2>&1
if errorlevel 1 goto nopython

:: Check if it's the Windows Store stub
python -c "import sys; exit(1 if 'WindowsApps' in sys.executable else 0)" >nul 2>&1
if errorlevel 1 goto nopython

echo Installing matplotlib...
pip install matplotlib -q

echo Starting monitor...
start "Sensor Simulator" /min cmd /c "timeout /t 3 >nul && sensor_sim\sensor_sim.exe"
python monitor\monitor.py
goto end

:nopython
echo.
echo =====================================================
echo  Python not found or not installed correctly!
echo.
echo  Please install Python 3 from:
echo  https://www.python.org/downloads/
echo.
echo  IMPORTANT: During install, check the box:
echo  [v] Add Python to PATH
echo =====================================================
echo.
pause
goto end

:end
