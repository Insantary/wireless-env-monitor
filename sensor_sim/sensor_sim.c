/*
 * 环境数据采集模拟器
 * 模拟ESP32-S3采集DHT11温湿度 + 光敏电阻数据
 * 通过TCP发送至Qt上位机
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  #include <windows.h>
  #define SLEEP_MS(ms) Sleep(ms)
  typedef SOCKET sock_t;
  #define INVALID  INVALID_SOCKET
  #define ERR      SOCKET_ERROR
#else
  #include <unistd.h>
  #include <sys/socket.h>
  #include <arpa/inet.h>
  #define SLEEP_MS(ms) usleep((ms)*1000)
  typedef int sock_t;
  #define INVALID  -1
  #define ERR      -1
#endif

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8888
#define INTERVAL_MS 2000

/* 模拟传感器值，带随机漂移 */
static float    sim_temp  = 25.0f;
static float    sim_humi  = 55.0f;
static float    sim_light = 60.0f;
static float    sim_pres  = 1013.25f;  /* BMP280 I2C 气压 hPa */
static unsigned sim_seq   = 0;

static void update_sensors(void) {
    sim_temp  += ((rand() % 21) - 10) * 0.1f;
    sim_humi  += ((rand() % 21) - 10) * 0.2f;
    sim_light += ((rand() % 21) - 10) * 0.5f;
    sim_pres  += ((rand() % 11) -  5) * 0.1f;

    if (sim_temp  < 10.0f) sim_temp  = 10.0f;
    if (sim_temp  > 40.0f) sim_temp  = 40.0f;
    if (sim_humi  < 20.0f) sim_humi  = 20.0f;
    if (sim_humi  > 95.0f) sim_humi  = 95.0f;
    if (sim_light <  0.0f) sim_light =  0.0f;
    if (sim_light >100.0f) sim_light =100.0f;
    if (sim_pres  <1005.0f) sim_pres = 1005.0f;
    if (sim_pres  >1025.0f) sim_pres = 1025.0f;
    sim_seq++;
}

int main(void) {
    srand((unsigned)time(NULL));

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif

    printf("[模拟器] 环境数据采集模拟器启动\n");
    printf("[模拟器] 目标: %s:%d\n\n", SERVER_IP, SERVER_PORT);

    while (1) {
        /* 建立TCP连接 */
        sock_t fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == INVALID) { printf("[错误] socket创建失败\n"); SLEEP_MS(3000); continue; }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(SERVER_PORT);
        addr.sin_addr.s_addr = inet_addr(SERVER_IP);

        printf("[模拟器] 连接服务器...\n");
        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == ERR) {
            printf("[模拟器] 连接失败，5秒后重试...\n");
#ifdef _WIN32
            closesocket(fd);
#else
            close(fd);
#endif
            SLEEP_MS(5000);
            continue;
        }
        printf("[模拟器] 已连接，开始发送数据\n\n");

        /* 发送循环 */
        while (1) {
            /* 检查是否有指令 */
            char cmd[32] = {0};
#ifdef _WIN32
            u_long mode = 1;
            ioctlsocket(fd, FIONBIO, &mode);
#endif
            int n = recv(fd, cmd, sizeof(cmd)-1, 0);
            if (n > 0) {
                cmd[n] = '\0';
                printf("[指令] 收到: %s", cmd);
            }
#ifdef _WIN32
            mode = 0;
            ioctlsocket(fd, FIONBIO, &mode);
#endif

            update_sensors();

            char buf[128];
            snprintf(buf, sizeof(buf),
                     "{\"seq\":%u,\"temp\":%.1f,\"humi\":%.1f,\"light\":%d,\"pres\":%.1f}\n",
                     sim_seq, sim_temp, sim_humi, (int)sim_light, sim_pres);

            int sent = send(fd, buf, (int)strlen(buf), 0);
            if (sent == ERR) {
                printf("[模拟器] 连接断开，重连中...\n");
                break;
            }
            printf("[TX] %s", buf);
            SLEEP_MS(INTERVAL_MS);
        }

#ifdef _WIN32
        closesocket(fd);
#else
        close(fd);
#endif
        SLEEP_MS(2000);
    }

#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
