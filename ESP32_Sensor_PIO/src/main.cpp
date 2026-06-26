#include <Arduino.h>
#include <WiFi.h>
#include <DHT.h>

// ========== 配置 ==========
const char* WIFI_SSID     = "HONOR";
const char* WIFI_PASSWORD = "12345678";
const char* SERVER_IP     = "10.5.11.57";
const int   SERVER_PORT   = 8888;
// ==========================

#define DHT_PIN   4
#define LIGHT_PIN 2
#define DHT_TYPE  DHT11

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient client;
bool collecting = true;

void connectWiFi() {
    Serial.printf("连接WiFi: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(500);
        Serial.print(".");
        retry++;
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("WiFi已连接，IP: %s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("WiFi连接失败，重启中...");
}

bool connectServer() {
    Serial.printf("连接服务器 %s:%d ...\n", SERVER_IP, SERVER_PORT);
    if (!client.connect(SERVER_IP, SERVER_PORT)) {
        Serial.println("服务器连接失败");
        return false;
    }
    Serial.println("服务器连接成功");
    return true;
}

int readLight() {
    int raw = analogRead(LIGHT_PIN);
    return constrain((4095 - raw) * 100 / 4095, 0, 100);
}

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    dht.begin();
    connectWiFi();
    connectServer();
}

void loop() {
    // 断线重连
    if (!client.connected()) {
        Serial.println("断线，5秒后重连...");
        delay(5000);
        if (WiFi.status() != WL_CONNECTED) connectWiFi();
        connectServer();
        return;
    }

    // 接收上位机指令
    while (client.available()) {
        String cmd = client.readStringUntil('\n');
        cmd.trim();
        if (cmd == "START") { collecting = true;  Serial.println("[CMD] START"); }
        if (cmd == "STOP")  { collecting = false; Serial.println("[CMD] STOP");  }
    }

    if (!collecting) { delay(500); return; }

    // 读传感器
    float temp  = dht.readTemperature();
    float humi  = dht.readHumidity();
    int   light = readLight();

    if (isnan(temp) || isnan(humi)) {
        Serial.println("DHT11读取失败");
        delay(2000);
        return;
    }

    // 发送JSON
    char buf[64];
    snprintf(buf, sizeof(buf), "{\"temp\":%.1f,\"humi\":%.1f,\"light\":%d}\n",
             temp, humi, light);
    client.print(buf);
    Serial.print("TX: ");
    Serial.print(buf);

    delay(2000);
}
