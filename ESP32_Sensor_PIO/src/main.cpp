/*
 * 无线环境数据采集监测系统 - ESP32-S3-WROOM-1U 固件
 *
 * 传感器接线：
 *   DHT11  DATA → GPIO4   (单总线温湿度)
 *   LDR    AO   → GPIO2   (ADC1_CH1 光照)
 *   BMP280 SDA  → GPIO21  (I2C 气压)
 *   BMP280 SCL  → GPIO22  (I2C 气压)
 *
 * 通信方式：
 *   WiFi TCP → 上位机 Qt 程序（端口8888）
 *   BLE      → 广播传感器数据（兼容手机App接收）
 *
 * JSON格式：{"seq":1,"temp":25.5,"humi":53.0,"light":60,"pres":1013.2}
 */

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <DHT.h>
#include <Adafruit_BMP280.h>

// ──────────────────────────────────────────────────────────────────────────────
// 配置区
// ──────────────────────────────────────────────────────────────────────────────
const char* WIFI_SSID     = "HONOR";
const char* WIFI_PASSWORD = "12345678";
const char* SERVER_IP     = "10.5.11.57";
const int   SERVER_PORT   = 8888;

#define DHT_PIN     4
#define LIGHT_PIN   2
#define DHT_TYPE    DHT11
#define BMP_SDA     21
#define BMP_SCL     22

// BLE 服务/特征 UUID（自定义）
#define BLE_SERVICE_UUID    "12345678-1234-1234-1234-123456789abc"
#define BLE_CHAR_UUID       "87654321-4321-4321-4321-cba987654321"

// ──────────────────────────────────────────────────────────────────────────────
// 全局对象
// ──────────────────────────────────────────────────────────────────────────────
DHT             dht(DHT_PIN, DHT_TYPE);
Adafruit_BMP280 bmp;
WiFiClient      client;

BLEServer*         bleServer    = nullptr;
BLECharacteristic* bleChar      = nullptr;
bool               bleConnected = false;

bool     collecting  = true;
bool     bmpOk       = false;
uint32_t seqNum      = 0;

// ──────────────────────────────────────────────────────────────────────────────
// BLE 回调（监控连接/断开）
// ──────────────────────────────────────────────────────────────────────────────
class MyBleCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer*)    override { bleConnected = true;  Serial.println("[BLE] 客户端已连接"); }
    void onDisconnect(BLEServer*) override {
        bleConnected = false;
        Serial.println("[BLE] 客户端断开，重新广播...");
        BLEDevice::getAdvertising()->start();
    }
};

// ──────────────────────────────────────────────────────────────────────────────
// 初始化 BLE
// ──────────────────────────────────────────────────────────────────────────────
void initBLE() {
    BLEDevice::init("ESP32-S3-Sensor");
    bleServer = BLEDevice::createServer();
    bleServer->setCallbacks(new MyBleCallbacks());

    BLEService *svc = bleServer->createService(BLE_SERVICE_UUID);
    bleChar = svc->createCharacteristic(
        BLE_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    bleChar->setValue("waiting...");
    svc->start();

    BLEAdvertising *adv = BLEDevice::getAdvertising();
    adv->addServiceUUID(BLE_SERVICE_UUID);
    adv->setScanResponse(true);
    adv->start();
    Serial.println("[BLE] 广播已启动，设备名: ESP32-S3-Sensor");
}

// ──────────────────────────────────────────────────────────────────────────────
// WiFi 连接
// ──────────────────────────────────────────────────────────────────────────────
void connectWiFi() {
    Serial.printf("[WiFi] 连接: %s\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry++ < 20) {
        delay(500); Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED)
        Serial.printf("[WiFi] 已连接，IP: %s\n", WiFi.localIP().toString().c_str());
    else
        Serial.println("[WiFi] 连接失败");
}

// ──────────────────────────────────────────────────────────────────────────────
// TCP 连接
// ──────────────────────────────────────────────────────────────────────────────
bool connectServer() {
    Serial.printf("[TCP] 连接 %s:%d ...\n", SERVER_IP, SERVER_PORT);
    if (!client.connect(SERVER_IP, SERVER_PORT)) {
        Serial.println("[TCP] 连接失败");
        return false;
    }
    Serial.println("[TCP] 连接成功");
    return true;
}

// ──────────────────────────────────────────────────────────────────────────────
// 光照读取（ADC1_CH1，不受WiFi干扰）
// ──────────────────────────────────────────────────────────────────────────────
int readLight() {
    int raw = analogRead(LIGHT_PIN);
    return constrain((4095 - raw) * 100 / 4095, 0, 100);
}

// ──────────────────────────────────────────────────────────────────────────────
// setup()
// ──────────────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    analogReadResolution(12);

    // I2C 初始化（BMP280 气压传感器）
    Wire.begin(BMP_SDA, BMP_SCL);
    bmpOk = bmp.begin(0x76);  // I2C 地址 0x76 或 0x77
    if (bmpOk) {
        bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                        Adafruit_BMP280::SAMPLING_X2,
                        Adafruit_BMP280::SAMPLING_X16,
                        Adafruit_BMP280::FILTER_X16,
                        Adafruit_BMP280::STANDBY_MS_500);
        Serial.println("[I2C] BMP280 初始化成功");
    } else {
        Serial.println("[I2C] BMP280 未检测到，使用模拟气压值");
    }

    // DHT11 初始化
    dht.begin();
    Serial.println("[DHT11] 传感器已初始化");

    // WiFi + TCP
    connectWiFi();
    connectServer();

    // BLE 广播
    initBLE();
}

// ──────────────────────────────────────────────────────────────────────────────
// loop()
// ──────────────────────────────────────────────────────────────────────────────
void loop() {
    // TCP 断线重连
    if (!client.connected()) {
        Serial.println("[TCP] 断线，5秒后重连...");
        delay(5000);
        if (WiFi.status() != WL_CONNECTED) connectWiFi();
        connectServer();
        return;
    }

    // 接收上位机指令
    while (client.available()) {
        String cmd = client.readStringUntil('\n');
        cmd.trim();
        if      (cmd == "START") { collecting = true;  Serial.println("[CMD] START"); }
        else if (cmd == "STOP")  { collecting = false; Serial.println("[CMD] STOP");  }
    }

    if (!collecting) { delay(500); return; }

    // ── 读取传感器 ──────────────────────────────────────────────────────────
    float temp  = dht.readTemperature();
    float humi  = dht.readHumidity();
    int   light = readLight();

    // BMP280 气压（I2C）；若无硬件则使用模拟值
    float pres;
    if (bmpOk) {
        pres = bmp.readPressure() / 100.0f;   // Pa → hPa
        pres = constrain(pres, 900.0f, 1100.0f);
    } else {
        // 模拟：标准大气压附近随机漂移（仿真用）
        static float simPres = 1013.25f;
        simPres += (random(-10, 11)) / 10.0f;
        simPres = constrain(simPres, 1005.0f, 1025.0f);
        pres = simPres;
    }

    if (isnan(temp) || isnan(humi)) {
        Serial.println("[DHT11] 读取失败，跳过本次");
        delay(2000);
        return;
    }

    seqNum++;

    // ── WiFi TCP 发送 JSON ──────────────────────────────────────────────────
    char buf[96];
    snprintf(buf, sizeof(buf),
             "{\"seq\":%lu,\"temp\":%.1f,\"humi\":%.1f,\"light\":%d,\"pres\":%.1f}\n",
             seqNum, temp, humi, light, pres);
    client.print(buf);
    Serial.printf("[TX] %s", buf);

    // ── BLE 通知（同步推送给蓝牙设备）─────────────────────────────────────
    if (bleChar) {
        bleChar->setValue(std::string(buf, strlen(buf) - 1));  // 去掉换行
        if (bleConnected) bleChar->notify();
    }

    delay(2000);
}
