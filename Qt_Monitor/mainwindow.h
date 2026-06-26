#pragma once
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QFile>
#include "tcpserver.h"
#include "settingsdialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onData(SensorData d);
    void onStatsUpdated(int total, int lost);
    void onClientConnected();
    void onClientDisconnected();
    void onStartStop();
    void onSettings();
    void onHistory();

private:
    void setupUI();
    void setupChart();
    void setupMenu();
    void checkAlarm(const SensorData &d);
    void logToCsv(const SensorData &d);
    void addChartPoint(QLineSeries *s, QValueAxis *axY, double val);

    TcpServer   *m_srv;
    AppSettings  m_settings;

    // 左侧数值标签
    QLabel      *m_lblStatus;
    QLabel      *m_lblTemp;
    QLabel      *m_lblHumi;
    QLabel      *m_lblLight;
    QLabel      *m_lblPres;
    QLabel      *m_lblAlarm;
    QLabel      *m_lblStats;
    QLabel      *m_lblBle;

    QPushButton *m_btnStartStop;

    QTextEdit   *m_log;

    // 四路图表
    QChartView  *m_chartTemp;
    QChartView  *m_chartHumi;
    QChartView  *m_chartLight;
    QChartView  *m_chartPres;

    QLineSeries *m_serTemp;
    QLineSeries *m_serHumi;
    QLineSeries *m_serLight;
    QLineSeries *m_serPres;

    QValueAxis  *m_axYTemp;
    QValueAxis  *m_axYHumi;
    QValueAxis  *m_axYLight;
    QValueAxis  *m_axYPres;

    int          m_xCount    = 0;
    bool         m_collecting = true;

    QFile        m_csvFile;
    QString      m_csvPath;
};
