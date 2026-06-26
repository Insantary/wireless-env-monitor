# 无线环境数据采集与监测系统

《嵌入式系统设计》课程设计 · 题目二

## 项目简介

基于 ESP32-S3-WROOM-1U 的无线环境数据采集监测系统，采集温度、湿度、光照数据，通过 WiFi TCP 传输至上位机实时显示。

```
ESP32-S3（DHT11 + 光敏电阻）
    ↓ WiFi TCP
上位机 Python 监控界面（实时曲线 + 报警 + CSV存储）
```

## 硬件接线

| 传感器 | 引脚 | ESP32-S3 |
|--------|------|----------|
| DHT11 DATA | — | GPIO4 |
| DHT11 VCC | — | 3.3V |
| DHT11 GND | — | GND |
| 光敏模块 AO | — | GPIO2 |
| 光敏模块 VCC | — | 3.3V |
| 光敏模块 GND | — | GND |

## 目录结构

```
├── ESP32_Sensor_PIO/       # ESP32-S3 固件（PlatformIO）
│   ├── platformio.ini
│   └── src/main.cpp
├── monitor/
│   └── monitor.py          # 上位机监控界面（Python）
├── sensor_sim/
│   ├── sensor_sim.c        # 传感器数据模拟器（C语言）
│   └── sensor_sim.exe      # Windows可执行文件
├── report_images/          # 报告配图
└── 课程设计报告_李欣.docx   # 完整课程设计报告
```

## 快速开始

### 方式一：模拟运行（无需硬件）

```bash
# 1. 启动上位机
python monitor/monitor.py

# 2. 启动模拟器
sensor_sim/sensor_sim.exe
```

### 方式二：真实硬件

1. 修改 `ESP32_Sensor_PIO/src/main.cpp` 中的 WiFi 名称、密码、服务器 IP
2. PlatformIO 烧录（COM7 / CH340）
3. 启动上位机 `python monitor/monitor.py`

## 上位机依赖

```bash
pip install matplotlib
```

Python 3.x + tkinter（内置）

## 功能特性

- 实时显示温度、湿度、光照三路数据及曲线
- 超阈值报警（温度>35°C / 湿度>80% / 光照<20%）
- 远程控制采集启停（START/STOP 指令）
- 数据自动保存至 `sensor_data.csv`
- 断线自动重连

## 开发环境

- 设备端：PlatformIO + Arduino Framework（ESP32-S3）
- 上位机：Python 3 + Tkinter + Matplotlib
- 通信协议：TCP，JSON 格式，2秒/包
