#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QTextStream>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>

static QChartView* makeChart(const QString &title, QLineSeries *&series,
                              QValueAxis *&axY, double yMin, double yMax,
                              const QString &unit, QColor color) {
    series = new QLineSeries();
    series->setColor(color);

    QValueAxis *axX = new QValueAxis();
    axX->setRange(0, 30);
    axX->setLabelFormat("%d");
    axX->setTitleText("采样点");

    axY = new QValueAxis();
    axY->setRange(yMin, yMax);
    axY->setTitleText(unit);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->addAxis(axX, Qt::AlignBottom);
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axX);
    series->attachAxis(axY);
    chart->setTitle(title);
    chart->legend()->hide();
    chart->setMargins(QMargins(4,4,4,4));

    QChartView *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(180);
    return view;
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("环境数据采集监测系统");
    resize(1000, 700);
    setupUI();
    setupChart();

    m_srv = new TcpServer(this);
    connect(m_srv, &TcpServer::dataReceived,       this, &MainWindow::onData);
    connect(m_srv, &TcpServer::clientConnected,    this, &MainWindow::onClientConnected);
    connect(m_srv, &TcpServer::clientDisconnected, this, &MainWindow::onClientDisconnected);

    if (m_srv->start(8888))
        m_log->append("[系统] TCP服务器已启动，监听端口 8888");
    else
        m_log->append("[错误] 端口8888启动失败");

    // 创建CSV
    m_csvFile.setFileName("sensor_data.csv");
    if (m_csvFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&m_csvFile);
        if (m_csvFile.size() == 0)
            s << "时间,温度(°C),湿度(%),光照(%)\n";
    }
}

void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // ---- 左侧：实时数值 + 控制 ----
    auto *grpVal = new QGroupBox("实时数值");
    auto *valLayout = new QGridLayout(grpVal);

    auto makeValLabel = [](const QString &text, int fontSize) {
        QLabel *l = new QLabel(text);
        QFont f = l->font(); f.setPointSize(fontSize); l->setFont(f);
        l->setAlignment(Qt::AlignCenter);
        return l;
    };

    valLayout->addWidget(makeValLabel("温度", 10),  0, 0);
    valLayout->addWidget(makeValLabel("湿度", 10),  1, 0);
    valLayout->addWidget(makeValLabel("光照", 10),  2, 0);

    m_lblTemp  = makeValLabel("--.-  °C", 18);
    m_lblHumi  = makeValLabel("--.-  %",  18);
    m_lblLight = makeValLabel("---   %",  18);
    m_lblTemp->setStyleSheet("color:#e74c3c");
    m_lblHumi->setStyleSheet("color:#2980b9");
    m_lblLight->setStyleSheet("color:#f39c12");

    valLayout->addWidget(m_lblTemp,  0, 1);
    valLayout->addWidget(m_lblHumi,  1, 1);
    valLayout->addWidget(m_lblLight, 2, 1);

    m_lblStatus = new QLabel("● 未连接");
    m_lblStatus->setStyleSheet("color:gray; font-weight:bold;");

    m_lblAlarm = new QLabel("");
    m_lblAlarm->setStyleSheet("color:red; font-weight:bold;");
    m_lblAlarm->setWordWrap(true);

    m_btnStartStop = new QPushButton("暂停采集");
    connect(m_btnStartStop, &QPushButton::clicked, this, &MainWindow::onStartStop);

    auto *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(grpVal);
    leftLayout->addWidget(m_lblStatus);
    leftLayout->addWidget(m_lblAlarm);
    leftLayout->addWidget(m_btnStartStop);
    leftLayout->addStretch();

    // ---- 右侧：日志 ----
    auto *grpLog = new QGroupBox("运行日志");
    auto *logLayout = new QVBoxLayout(grpLog);
    m_log = new QTextEdit();
    m_log->setReadOnly(true);
    m_log->setMaximumWidth(280);
    logLayout->addWidget(m_log);

    // ---- 图表区 ----
    auto *chartsLayout = new QVBoxLayout();
    // placeholder，真正的chart在setupChart后加入
    m_chartTemp  = nullptr;
    m_chartHumi  = nullptr;
    m_chartLight = nullptr;

    // ---- 主布局 ----
    auto *rightLayout = new QVBoxLayout();
    rightLayout->addLayout(chartsLayout);

    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->addLayout(leftLayout, 0);
    mainLayout->addLayout(rightLayout, 1);
    mainLayout->addWidget(grpLog, 0);

    // 保存chartsLayout指针供setupChart使用
    central->setProperty("chartsLayout", QVariant::fromValue((void*)rightLayout));
}

void MainWindow::setupChart() {
    m_chartTemp  = makeChart("温度曲线", m_serTemp,  m_axYTemp,  -10, 50, "°C",  QColor("#e74c3c"));
    m_chartHumi  = makeChart("湿度曲线", m_serHumi,  m_axYHumi,  0,  100, "%",   QColor("#2980b9"));
    m_chartLight = makeChart("光照曲线", m_serLight, m_axYLight, 0,  100, "%",   QColor("#f39c12"));

    // 找到中间那个QVBoxLayout
    auto *rightLayout = (QVBoxLayout*)centralWidget()->property("chartsLayout").value<void*>();
    rightLayout->addWidget(m_chartTemp);
    rightLayout->addWidget(m_chartHumi);
    rightLayout->addWidget(m_chartLight);
}

void MainWindow::addChartPoint(QLineSeries *s, QValueAxis *axY, double val) {
    Q_UNUSED(axY)
    s->append(m_xCount, val);
    // 保留最近30个点
    if (s->count() > 30)
        s->remove(0);
    // 让X轴跟随
    auto *chart = s->chart();
    auto axes = chart->axes(Qt::Horizontal);
    if (!axes.isEmpty()) {
        int xEnd = qMax(30, m_xCount);
        axes[0]->setRange(xEnd - 30, xEnd);
    }
}

void MainWindow::onData(SensorData d) {
    m_xCount++;

    m_lblTemp->setText(QString("%1  °C").arg(d.temp, 0, 'f', 1));
    m_lblHumi->setText(QString("%1  %").arg(d.humi, 0, 'f', 1));
    m_lblLight->setText(QString("%1  %").arg(d.light));

    addChartPoint(m_serTemp,  m_axYTemp,  d.temp);
    addChartPoint(m_serHumi,  m_axYHumi,  d.humi);
    addChartPoint(m_serLight, m_axYLight, d.light);

    checkAlarm(d);
    logToCsv(d);

    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_log->append(QString("[%1] 温度:%2°C 湿度:%3% 光照:%4%")
                      .arg(time)
                      .arg(d.temp, 0,'f',1)
                      .arg(d.humi, 0,'f',1)
                      .arg(d.light));
}

void MainWindow::checkAlarm(const SensorData &d) {
    QStringList alarms;
    if (d.temp > TEMP_MAX)
        alarms << QString("⚠ 温度过高：%1°C").arg(d.temp, 0,'f',1);
    if (d.humi > HUMI_MAX)
        alarms << QString("⚠ 湿度过高：%1%").arg(d.humi, 0,'f',1);
    if (d.light < LIGHT_MIN)
        alarms << QString("⚠ 光照过低：%1%").arg(d.light);

    m_lblAlarm->setText(alarms.join("\n"));
}

void MainWindow::logToCsv(const SensorData &d) {
    if (!m_csvFile.isOpen()) return;
    QTextStream s(&m_csvFile);
    s << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
      << "," << d.temp << "," << d.humi << "," << d.light << "\n";
}

void MainWindow::onStartStop() {
    m_collecting = !m_collecting;
    m_srv->sendCmd(m_collecting ? "START" : "STOP");
    m_btnStartStop->setText(m_collecting ? "暂停采集" : "继续采集");
    m_log->append(m_collecting ? "[控制] 已发送 START" : "[控制] 已发送 STOP");
}

void MainWindow::onClientConnected() {
    m_lblStatus->setText("● 设备已连接");
    m_lblStatus->setStyleSheet("color:green; font-weight:bold;");
    m_log->append("[系统] ESP32-S3 已连接");
}

void MainWindow::onClientDisconnected() {
    m_lblStatus->setText("● 设备已断开");
    m_lblStatus->setStyleSheet("color:red; font-weight:bold;");
    m_log->append("[系统] ESP32-S3 已断开，等待重连...");
}
