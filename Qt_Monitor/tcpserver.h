#pragma once
#include <QTcpServer>
#include <QTcpSocket>

struct SensorData {
    float temp;
    float humi;
    int   light;
};

class TcpServer : public QObject {
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);
    bool start(quint16 port);
    void stop();
    void sendCmd(const QString &cmd);

signals:
    void dataReceived(SensorData data);
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
};
