#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QTimer>
#include <QFile>
#include "tcpserver.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onData(SensorData d);
    void onClientConnected();
    void onClientDisconnected();
    void onStartStop();

private:
    void setupUI();
    void setupChart();
    void checkAlarm(const SensorData &d);
    void logToCsv(const SensorData &d);
    void addChartPoint(QLineSeries *s, QValueAxis *axY, double val);

    TcpServer   *m_srv;

    // 状态标签
    QLabel      *m_lblStatus;
    QLabel      *m_lblTemp;
    QLabel      *m_lblHumi;
    QLabel      *m_lblLight;
    QLabel      *m_lblAlarm;

    // 按钮
    QPushButton *m_btnStartStop;

    // 日志
    QTextEdit   *m_log;

    // 图表
    QChartView  *m_chartTemp;
    QChartView  *m_chartHumi;
    QChartView  *m_chartLight;

    QLineSeries *m_serTemp;
    QLineSeries *m_serHumi;
    QLineSeries *m_serLight;

    QValueAxis  *m_axYTemp;
    QValueAxis  *m_axYHumi;
    QValueAxis  *m_axYLight;

    int          m_xCount = 0;
    bool         m_collecting = true;

    // 报警阈值
    static constexpr float TEMP_MAX  = 35.0f;
    static constexpr float HUMI_MAX  = 80.0f;
    static constexpr int   LIGHT_MIN = 20;

    QFile        m_csvFile;
};
