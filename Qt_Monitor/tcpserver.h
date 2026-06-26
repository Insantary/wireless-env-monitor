#pragma once
#include <QTcpServer>
#include <QTcpSocket>

struct SensorData {
    float temp  = 0;
    float humi  = 0;
    int   light = 0;
    float pres  = 0;   // 气压 hPa (BMP280 I2C)
    int   seq   = -1;  // 序列号，用于丢包检测
};

struct PacketStats {
    int total  = 0;
    int lost   = 0;
    int lastSeq = -1;
};

class TcpServer : public QObject {
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);
    bool start(quint16 port);
    void stop();
    void sendCmd(const QString &cmd);
    PacketStats stats() const { return m_stats; }

signals:
    void dataReceived(SensorData data);
    void statsUpdated(int total, int lost);
    void clientConnected();
    void clientDisconnected();

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpServer  *m_server  = nullptr;
    QTcpSocket  *m_client  = nullptr;
    QByteArray   m_buf;
    PacketStats  m_stats;
};
