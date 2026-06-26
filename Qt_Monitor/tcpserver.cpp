#include "tcpserver.h"
#include <QJsonDocument>
#include <QJsonObject>

TcpServer::TcpServer(QObject *parent) : QObject(parent) {
    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

bool TcpServer::start(quint16 port) {
    return m_server->listen(QHostAddress::Any, port);
}

void TcpServer::stop() {
    if (m_client) m_client->disconnectFromHost();
    m_server->close();
}

void TcpServer::sendCmd(const QString &cmd) {
    if (m_client && m_client->state() == QAbstractSocket::ConnectedState)
        m_client->write((cmd + "\n").toUtf8());
}

void TcpServer::onNewConnection() {
    if (m_client) {
        m_client->disconnectFromHost();
        m_client->deleteLater();
    }
    m_client = m_server->nextPendingConnection();
    m_stats  = PacketStats{};   // 新连接重置统计
    connect(m_client, &QTcpSocket::readyRead,    this, &TcpServer::onReadyRead);
    connect(m_client, &QTcpSocket::disconnected, this, &TcpServer::onDisconnected);
    emit clientConnected();
}

void TcpServer::onReadyRead() {
    m_buf += m_client->readAll();
    while (m_buf.contains('\n')) {
        int idx = m_buf.indexOf('\n');
        QByteArray line = m_buf.left(idx).trimmed();
        m_buf.remove(0, idx + 1);
        if (line.isEmpty()) continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError) continue;

        QJsonObject obj = doc.object();
        SensorData d;
        d.temp  = (float)obj["temp"].toDouble();
        d.humi  = (float)obj["humi"].toDouble();
        d.light = obj["light"].toInt();
        d.pres  = (float)obj.value("pres").toDouble(1013.25);  // 默认标准气压
        d.seq   = obj.value("seq").toInt(-1);

        // 丢包检测
        m_stats.total++;
        if (d.seq >= 0 && m_stats.lastSeq >= 0) {
            int expected = m_stats.lastSeq + 1;
            if (d.seq > expected)
                m_stats.lost += d.seq - expected;
        }
        if (d.seq >= 0) m_stats.lastSeq = d.seq;

        emit dataReceived(d);
        emit statsUpdated(m_stats.total, m_stats.lost);
    }
}

void TcpServer::onDisconnected() {
    emit clientDisconnected();
    m_client->deleteLater();
    m_client = nullptr;
}
