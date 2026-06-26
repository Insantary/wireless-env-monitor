"""
无线环境数据采集监测系统 - 上位机监控界面
TCP服务端，实时显示温度/湿度/光照，支持报警、数据存储、历史曲线
"""
import tkinter as tk
from tkinter import ttk, font
import socket
import threading
import json
import csv
import os
from datetime import datetime
from collections import deque
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt

# ========== 配置 ==========
HOST        = "0.0.0.0"
PORT        = 8888
MAX_POINTS  = 30       # 曲线最多显示点数
CSV_FILE    = os.path.join(os.path.dirname(__file__), "sensor_data.csv")
TEMP_MAX    = 35.0
HUMI_MAX    = 80.0
LIGHT_MIN   = 20
# ==========================

plt.rcParams["font.family"] = ["Microsoft YaHei", "SimHei", "sans-serif"]
plt.rcParams["axes.unicode_minus"] = False


class MonitorApp:
    def __init__(self, root):
        self.root = root
        self.root.title("无线环境数据采集监测系统")
        self.root.geometry("1100x680")
        self.root.resizable(True, True)

        self.collecting = True
        self.client_sock = None
        self.client_lock = threading.Lock()

        self.times  = deque(maxlen=MAX_POINTS)
        self.temps  = deque(maxlen=MAX_POINTS)
        self.humis  = deque(maxlen=MAX_POINTS)
        self.lights = deque(maxlen=MAX_POINTS)
        self.x_idx  = 0

        self._init_csv()
        self._build_ui()
        self._start_server()

    # ------------------------------------------------------------------ UI --
    def _build_ui(self):
        BG = "#1e1e2e"
        self.root.configure(bg=BG)

        # 左侧面板
        left = tk.Frame(self.root, bg=BG, width=220)
        left.pack(side=tk.LEFT, fill=tk.Y, padx=10, pady=10)
        left.pack_propagate(False)

        tk.Label(left, text="环境监测系统", bg=BG, fg="#cdd6f4",
                 font=("Microsoft YaHei", 14, "bold")).pack(pady=(10,20))

        # 状态
        self.lbl_status = tk.Label(left, text="● 未连接", bg=BG,
                                   fg="#6c7086", font=("Microsoft YaHei", 11, "bold"))
        self.lbl_status.pack(pady=4)

        # 数值卡片
        self.lbl_temp  = self._val_card(left, "温  度", "--.-", "°C",  "#f38ba8")
        self.lbl_humi  = self._val_card(left, "湿  度", "--.-", "%",   "#89b4fa")
        self.lbl_light = self._val_card(left, "光  照", "---",  "%",   "#f9e2af")

        # 报警区
        self.lbl_alarm = tk.Label(left, text="", bg=BG, fg="#f38ba8",
                                  font=("Microsoft YaHei", 10), wraplength=200,
                                  justify=tk.LEFT)
        self.lbl_alarm.pack(pady=8, fill=tk.X)

        # 按钮
        self.btn = tk.Button(left, text="⏸  暂停采集",
                             command=self._toggle_collect,
                             bg="#313244", fg="#cdd6f4",
                             font=("Microsoft YaHei", 11),
                             relief=tk.FLAT, cursor="hand2", pady=6)
        self.btn.pack(fill=tk.X, pady=4)

        # 日志
        tk.Label(left, text="运行日志", bg=BG, fg="#6c7086",
                 font=("Microsoft YaHei", 9)).pack(anchor=tk.W, pady=(16,2))
        self.log = tk.Text(left, bg="#181825", fg="#a6adc8",
                           font=("Consolas", 8), relief=tk.FLAT,
                           state=tk.DISABLED, wrap=tk.WORD)
        self.log.pack(fill=tk.BOTH, expand=True)

        # 右侧图表
        right = tk.Frame(self.root, bg=BG)
        right.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0,10), pady=10)

        self.fig = Figure(facecolor="#1e1e2e")
        self.ax_t = self.fig.add_subplot(311)
        self.ax_h = self.fig.add_subplot(312)
        self.ax_l = self.fig.add_subplot(313)
        self.fig.subplots_adjust(hspace=0.55)

        self._style_ax(self.ax_t, "温度 (°C)",  "#f38ba8", 0,  45)
        self._style_ax(self.ax_h, "湿度 (%)",   "#89b4fa", 0, 100)
        self._style_ax(self.ax_l, "光照 (%)",   "#f9e2af", 0, 100)

        self.line_t, = self.ax_t.plot([], [], color="#f38ba8", lw=2)
        self.line_h, = self.ax_h.plot([], [], color="#89b4fa", lw=2)
        self.line_l, = self.ax_l.plot([], [], color="#f9e2af", lw=2)

        self.canvas = FigureCanvasTkAgg(self.fig, master=right)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    def _val_card(self, parent, label, val, unit, color):
        f = tk.Frame(parent, bg="#313244", pady=8)
        f.pack(fill=tk.X, pady=4)
        tk.Label(f, text=label, bg="#313244", fg="#6c7086",
                 font=("Microsoft YaHei", 9)).pack()
        lbl = tk.Label(f, text=f"{val} {unit}", bg="#313244", fg=color,
                       font=("Microsoft YaHei", 20, "bold"))
        lbl.pack()
        return lbl

    def _style_ax(self, ax, title, color, ymin, ymax):
        ax.set_facecolor("#181825")
        ax.set_title(title, color=color, fontsize=10, loc="left", pad=4)
        ax.set_ylim(ymin, ymax)
        ax.tick_params(colors="#6c7086", labelsize=7)
        ax.spines[:].set_color("#313244")
        ax.set_xlim(0, MAX_POINTS)

    # --------------------------------------------------------------- Server --
    def _start_server(self):
        self._log("[系统] TCP服务器启动，端口 8888")
        t = threading.Thread(target=self._server_loop, daemon=True)
        t.start()

    def _server_loop(self):
        srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        srv.bind((HOST, PORT))
        srv.listen(1)
        while True:
            conn, addr = srv.accept()
            self._log(f"[系统] 客户端连接: {addr[0]}")
            self.root.after(0, self._set_connected, True)
            with self.client_lock:
                self.client_sock = conn
            self._recv_loop(conn)
            with self.client_lock:
                self.client_sock = None
            self._log("[系统] 客户端断开，等待重连...")
            self.root.after(0, self._set_connected, False)

    def _recv_loop(self, conn):
        buf = b""
        while True:
            try:
                chunk = conn.recv(1024)
                if not chunk:
                    break
                buf += chunk
                while b"\n" in buf:
                    line, buf = buf.split(b"\n", 1)
                    line = line.strip()
                    if not line:
                        continue
                    try:
                        d = json.loads(line)
                        self.root.after(0, self._on_data, d)
                    except json.JSONDecodeError:
                        pass
            except Exception:
                break

    def _send_cmd(self, cmd):
        with self.client_lock:
            if self.client_sock:
                try:
                    self.client_sock.sendall((cmd + "\n").encode())
                except Exception:
                    pass

    # --------------------------------------------------------------- Data ---
    def _on_data(self, d):
        temp  = d.get("temp",  0.0)
        humi  = d.get("humi",  0.0)
        light = d.get("light", 0)

        self.lbl_temp.config(text=f"{temp:.1f} °C")
        self.lbl_humi.config(text=f"{humi:.1f} %")
        self.lbl_light.config(text=f"{light} %")

        self.x_idx += 1
        self.temps.append(temp)
        self.humis.append(humi)
        self.lights.append(light)

        xs = list(range(len(self.temps)))
        self.line_t.set_data(xs, list(self.temps))
        self.line_h.set_data(xs, list(self.humis))
        self.line_l.set_data(xs, list(self.lights))
        for ax in (self.ax_t, self.ax_h, self.ax_l):
            ax.set_xlim(0, max(MAX_POINTS, len(xs)))
        self.canvas.draw_idle()

        self._check_alarm(temp, humi, light)
        self._save_csv(temp, humi, light)
        now = datetime.now().strftime("%H:%M:%S")
        self._log(f"[{now}] 温:{temp:.1f}°C 湿:{humi:.1f}% 光:{light}%")

    def _check_alarm(self, temp, humi, light):
        alarms = []
        if temp  > TEMP_MAX:  alarms.append(f"⚠ 温度过高: {temp:.1f}°C")
        if humi  > HUMI_MAX:  alarms.append(f"⚠ 湿度过高: {humi:.1f}%")
        if light < LIGHT_MIN: alarms.append(f"⚠ 光照过低: {light}%")
        self.lbl_alarm.config(text="\n".join(alarms))

    # --------------------------------------------------------------- UI ops -
    def _toggle_collect(self):
        self.collecting = not self.collecting
        if self.collecting:
            self._send_cmd("START")
            self.btn.config(text="⏸  暂停采集")
            self._log("[控制] 发送 START")
        else:
            self._send_cmd("STOP")
            self.btn.config(text="▶  继续采集")
            self._log("[控制] 发送 STOP")

    def _set_connected(self, connected):
        if connected:
            self.lbl_status.config(text="● 已连接", fg="#a6e3a1")
        else:
            self.lbl_status.config(text="● 未连接", fg="#6c7086")
            self.lbl_temp.config(text="--.- °C")
            self.lbl_humi.config(text="--.- %")
            self.lbl_light.config(text="--- %")

    def _log(self, msg):
        self.log.config(state=tk.NORMAL)
        self.log.insert(tk.END, msg + "\n")
        self.log.see(tk.END)
        self.log.config(state=tk.DISABLED)

    # --------------------------------------------------------------- CSV ----
    def _init_csv(self):
        if not os.path.exists(CSV_FILE):
            with open(CSV_FILE, "w", newline="", encoding="utf-8") as f:
                csv.writer(f).writerow(["时间", "温度(°C)", "湿度(%)", "光照(%)"])

    def _save_csv(self, temp, humi, light):
        with open(CSV_FILE, "a", newline="", encoding="utf-8") as f:
            csv.writer(f).writerow([
                datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                f"{temp:.1f}", f"{humi:.1f}", light])


if __name__ == "__main__":
    root = tk.Tk()
    app = MonitorApp(root)
    root.mainloop()
