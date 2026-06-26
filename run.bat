@echo off
chcp 65001 >nul
echo ============================================
echo   Wireless Sensor Monitor - Simulation Mode
echo ============================================
echo.

echo [1/2] Installing dependencies (matplotlib)...
pip install matplotlib -q
if errorlevel 1 (
    echo ERROR: pip failed. Please install Python 3.x first.
    pause
    exit /b 1
)

echo [2/2] Starting monitor UI...
echo       Simulator will auto-connect in 3 seconds.
echo.
start "Sensor Simulator" /min cmd /c "timeout /t 3 >nul && sensor_sim\sensor_sim.exe"
python monitor\monitor.py

pause
