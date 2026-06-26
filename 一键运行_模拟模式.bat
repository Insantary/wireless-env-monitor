@echo off
chcp 65001 >nul
echo ============================================
echo   无线环境数据采集监测系统 - 模拟运行模式
echo ============================================
echo.

echo [1/2] 安装依赖（matplotlib）...
pip install matplotlib -q
if errorlevel 1 (
    echo 错误：pip安装失败，请确认已安装Python 3.x
    pause
    exit /b 1
)

echo [2/2] 启动上位机监控界面...
echo       上位机启动后，再等3秒会自动连上模拟器
echo.
start "传感器模拟器" /min cmd /c "timeout /t 3 >nul && sensor_sim\sensor_sim.exe"
python monitor\monitor.py

pause
