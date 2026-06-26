#include "mainwindow.h"
#include "historydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QTextStream>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>

// ─── 工具：创建一个折线图 ────────────────────────────────────────────────────
static QChartView* makeChart(const QString &title,
                              QLineSeries *&series, QValueAxis *&axY,
                              double yMin, double yMax,
                              const QString &unit, QColor color)
{
    series = new QLineSeries();
    series->setColor(color);

    auto *axX = new QValueAxis();
    axX->setRange(0, 30);
    axX->setLabelFormat("%d");
    axX->setTitleText("采样点");

    axY = new QValueAxis();
    axY->setRange(yMin, yMax);
    axY->setTitleText(unit);

    auto *chart = new QChart();
    chart->addSeries(series);
    chart->addAxis(axX, Qt::AlignBottom);
    chart->addAxis(axY, Qt::AlignLeft);
    series->attachAxis(axX);
    series->attachAxis(axY);
    chart->setTitle(title);
    chart->legend()->hide();
    chart->setMargins(QMargins(4, 4, 4, 4));

    auto *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setMinimumHeight(160);
    return view;
}

// ─── 构造函数 ────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setWindowTitle("无线环境数据采集监测系统  v2.0 (Qt)");
    resize(1200, 750);

    setupMenu();
    setupUI();
    setupChart();

    m_srv = new TcpServer(this);
    connect(m_srv, &TcpServer::dataReceived,       this, &MainWindow::onData);
    connect(m_srv, &TcpServer::statsUpdated,       this, &MainWindow::onStatsUpdated);
    connect(m_srv, &TcpServer::clientConnected,    this, &MainWindow::onClientConnected);
    connect(m_srv, &TcpServer::clientDisconnected, this, &MainWindow::onClientDisconnected);

    if (m_srv->start(m_settings.port))
        m_log->append(QString("[系统] TCP服务器已启动，监听端口 %1").arg(m_settings.port));
    else
        m_log->append("[错误] TCP端口启动失败，请检查端口是否被占用");

    // CSV 文件
    m_csvPath = "sensor_data.csv";
    m_csvFile.setFileName(m_csvPath);
    if (m_csvFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream s(&m_csvFile);
        if (m_csvFile.size() == 0)
            s << "时间,温度(°C),湿度(%),光照(%),气压(hPa)\n";
    }
    m_log->append("[系统] 数据文件：" + m_csvPath);
}

// ─── 菜单栏 ──────────────────────────────────────────────────────────────────
void MainWindow::setupMenu()
{
    auto *fileMenu = menuBar()->addMenu("文件(&F)");
    auto *actSettings = fileMenu->addAction("参数配置(&S)...");
    fileMenu->addSeparator();
    auto *actQuit = fileMenu->addAction("退出(&Q)");
    connect(actSettings, &QAction::triggered, this, &MainWindow::onSettings);
    connect(actQuit,     &QAction::triggered, this, &QWidget::close);

    auto *viewMenu = menuBar()->addMenu("查询(&V)");
    auto *actHistory = viewMenu->addAction("历史数据查询(&H)...");
    connect(actHistory, &QAction::triggered, this, &MainWindow::onHistory);

    auto *helpMenu = menuBar()->addMenu("帮助(&H)");
    auto *actAbout = helpMenu->addAction("关于(&A)");
    connect(actAbout, &QAction::triggered, this, [this]{
        m_log->append("[系统] 无线环境数据采集监测系统 v2.0 | Qt 6.5 + ESP32-S3");
    });
}

// ─── UI 布局 ─────────────────────────────────────────────────────────────────
void MainWindow::setupUI()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto makeValLabel = [](const QString &text, int fontSize, const QString &style = "") {
        auto *l = new QLabel(text);
        QFont f = l->font(); f.setPointSize(fontSize); l->setFont(f);
        l->setAlignment(Qt::AlignCenter);
        if (!style.isEmpty()) l->setStyleSheet(style);
        return l;
    };

    // ── 实时数值面板 ──────────────────────────────────────────────────────────
    auto *grpVal    = new QGroupBox("实时数值");
    auto *valLayout = new QGridLayout(grpVal);

    valLayout->addWidget(makeValLabel("温  度", 9), 0, 0);
    valLayout->addWidget(makeValLabel("湿  度", 9), 1, 0);
    valLayout->addWidget(makeValLabel("光  照", 9), 2, 0);
    valLayout->addWidget(makeValLabel("气  压", 9), 3, 0);

    m_lblTemp  = makeValLabel("--.-  °C",  18, "color:#e74c3c; font-weight:bold;");
    m_lblHumi  = makeValLabel("--.-  %",   18, "color:#2980b9; font-weight:bold;");
    m_lblLight = makeValLabel("---   %",   18, "color:#f39c12; font-weight:bold;");
    m_lblPres  = makeValLabel("----  hPa", 15, "color:#27ae60; font-weight:bold;");

    valLayout->addWidget(m_lblTemp,  0, 1);
    valLayout->addWidget(m_lblHumi,  1, 1);
    valLayout->addWidget(m_lblLight, 2, 1);
    valLayout->addWidget(m_lblPres,  3, 1);

    // ── 状态 / 报警 / 统计 ────────────────────────────────────────────────────
    m_lblStatus = new QLabel("● 未连接");
    m_lblStatus->setStyleSheet("color:gray; font-weight:bold;");

    m_lblBle = new QLabel("BLE: 未启用");
    m_lblBle->setStyleSheet("color:#8e44ad; font-size:11px;");

    m_lblAlarm = new QLabel("");
    m_lblAlarm->setStyleSheet("color:red; font-weight:bold;");
    m_lblAlarm->setWordWrap(true);

    m_lblStats = new QLabel("收包: 0  丢包: 0");
    m_lblStats->setStyleSheet("color:#555; font-size:11px;");

    // ── 控制按钮 ─────────────────────────────────────────────────────────────
    m_btnStartStop = new QPushButton("暂停采集");
    m_btnStartStop->setMinimumHeight(32);
    connect(m_btnStartStop, &QPushButton::clicked, this, &MainWindow::onStartStop);

    auto *btnSettings = new QPushButton("参数配置");
    btnSettings->setMinimumHeight(32);
    connect(btnSettings, &QPushButton::clicked, this, &MainWindow::onSettings);

    auto *btnHistory = new QPushButton("历史查询");
    btnHistory->setMinimumHeight(32);
    connect(btnHistory, &QPushButton::clicked, this, &MainWindow::onHistory);

    // ── 左侧布局 ─────────────────────────────────────────────────────────────
    auto *leftLayout = new QVBoxLayout();
    leftLayout->addWidget(grpVal);
    leftLayout->addWidget(m_lblStatus);
    leftLayout->addWidget(m_lblBle);
    leftLayout->addWidget(m_lblAlarm);
    leftLayout->addWidget(m_lblStats);
    leftLayout->addWidget(m_btnStartStop);
    leftLayout->addWidget(btnSettings);
    leftLayout->addWidget(btnHistory);
    leftLayout->addStretch();

    // ── 右侧日志 ─────────────────────────────────────────────────────────────
    auto *grpLog    = new QGroupBox("运行日志");
    auto *logLayout = new QVBoxLayout(grpLog);
    m_log = new QTextEdit();
    m_log->setReadOnly(true);
    m_log->setFixedWidth(260);
    logLayout->addWidget(m_log);

    // ── 图表占位，setupChart后填充 ────────────────────────────────────────────
    auto *chartsLayout = new QVBoxLayout();
    central->setProperty("chartsLayout", QVariant::fromValue((void*)chartsLayout));

    // ── 主布局 ───────────────────────────────────────────────────────────────
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->addLayout(leftLayout, 0);
    mainLayout->addLayout(chartsLayout, 1);
    mainLayout->addWidget(grpLog, 0);
}

// ─── 图表初始化 ──────────────────────────────────────────────────────────────
void MainWindow::setupChart()
{
    m_chartTemp  = makeChart("温度曲线 (°C)", m_serTemp,  m_axYTemp,  -10, 50,   "°C",  QColor("#e74c3c"));
    m_chartHumi  = makeChart("湿度曲线 (%)",  m_serHumi,  m_axYHumi,   0, 100,   "%",   QColor("#2980b9"));
    m_chartLight = makeChart("光照曲线 (%)",  m_serLight, m_axYLight,  0, 100,   "%",   QColor("#f39c12"));
    m_chartPres  = makeChart("气压曲线 (hPa)",m_serPres,  m_axYPres, 960, 1060,  "hPa", QColor("#27ae60"));

    auto *chartsLayout = (QVBoxLayout*)centralWidget()->property("chartsLayout").value<void*>();
    chartsLayout->addWidget(m_chartTemp);
    chartsLayout->addWidget(m_chartHumi);
    chartsLayout->addWidget(m_chartLight);
    chartsLayout->addWidget(m_chartPres);
}

// ─── 图表点追加 ──────────────────────────────────────────────────────────────
void MainWindow::addChartPoint(QLineSeries *s, QValueAxis *axY, double val)
{
    Q_UNUSED(axY)
    s->append(m_xCount, val);
    if (s->count() > 30) s->remove(0);
    auto *chart = s->chart();
    auto axes = chart->axes(Qt::Horizontal);
    if (!axes.isEmpty()) {
        int xEnd = qMax(30, m_xCount);
        axes[0]->setRange(xEnd - 30, xEnd);
    }
}

// ─── 接收数据 ────────────────────────────────────────────────────────────────
void MainWindow::onData(SensorData d)
{
    m_xCount++;
    m_lblTemp->setText(QString("%1  °C").arg(d.temp,  0, 'f', 1));
    m_lblHumi->setText(QString("%1  %").arg(d.humi,   0, 'f', 1));
    m_lblLight->setText(QString("%1  %").arg(d.light));
    m_lblPres->setText(QString("%1  hPa").arg(d.pres, 0, 'f', 1));

    addChartPoint(m_serTemp,  m_axYTemp,  d.temp);
    addChartPoint(m_serHumi,  m_axYHumi,  d.humi);
    addChartPoint(m_serLight, m_axYLight, d.light);
    addChartPoint(m_serPres,  m_axYPres,  d.pres);

    checkAlarm(d);
    logToCsv(d);

    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");
    m_log->append(QString("[%1] T:%2°C H:%3% L:%4% P:%5hPa")
                      .arg(time)
                      .arg(d.temp, 0,'f',1)
                      .arg(d.humi, 0,'f',1)
                      .arg(d.light)
                      .arg(d.pres, 0,'f',1));
}

// ─── 统计更新 ────────────────────────────────────────────────────────────────
void MainWindow::onStatsUpdated(int total, int lost)
{
    float rate = total > 0 ? lost * 100.0f / total : 0;
    m_lblStats->setText(QString("收包: %1  丢包: %2  丢包率: %3%")
                            .arg(total).arg(lost).arg(rate, 0, 'f', 1));
}

// ─── 报警检测 ────────────────────────────────────────────────────────────────
void MainWindow::checkAlarm(const SensorData &d)
{
    QStringList alarms;
    if (d.temp > m_settings.tempMax)
        alarms << QString("⚠ 温度过高：%1°C (阈值 %2°C)").arg(d.temp,0,'f',1).arg(m_settings.tempMax,0,'f',1);
    if (d.humi > m_settings.humiMax)
        alarms << QString("⚠ 湿度过高：%1% (阈值 %2%)").arg(d.humi,0,'f',1).arg(m_settings.humiMax,0,'f',1);
    if (d.light < m_settings.lightMin)
        alarms << QString("⚠ 光照过低：%1% (阈值 %2%)").arg(d.light).arg(m_settings.lightMin);
    if (d.pres < m_settings.presMin || d.pres > m_settings.presMax)
        alarms << QString("⚠ 气压异常：%1hPa").arg(d.pres,0,'f',1);
    m_lblAlarm->setText(alarms.join("\n"));
}

// ─── CSV存储 ─────────────────────────────────────────────────────────────────
void MainWindow::logToCsv(const SensorData &d)
{
    if (!m_csvFile.isOpen()) return;
    QTextStream s(&m_csvFile);
    s << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
      << "," << d.temp << "," << d.humi << "," << d.light << "," << d.pres << "\n";
}

// ─── 采集启停 ────────────────────────────────────────────────────────────────
void MainWindow::onStartStop()
{
    m_collecting = !m_collecting;
    m_srv->sendCmd(m_collecting ? "START" : "STOP");
    m_btnStartStop->setText(m_collecting ? "暂停采集" : "继续采集");
    m_log->append(m_collecting ? "[控制] 已发送 START" : "[控制] 已发送 STOP");
}

// ─── 参数配置对话框 ──────────────────────────────────────────────────────────
void MainWindow::onSettings()
{
    SettingsDialog dlg(m_settings, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_settings = dlg.settings();
        m_log->append(QString("[配置] 阈值已更新：T<%1°C H<%2% L>%3% P:%4~%5hPa")
                          .arg(m_settings.tempMax).arg(m_settings.humiMax)
                          .arg(m_settings.lightMin).arg(m_settings.presMin).arg(m_settings.presMax));
    }
}

// ─── 历史数据查询 ────────────────────────────────────────────────────────────
void MainWindow::onHistory()
{
    HistoryDialog dlg(m_csvPath, this);
    dlg.exec();
}

// ─── 连接状态 ────────────────────────────────────────────────────────────────
void MainWindow::onClientConnected()
{
    m_lblStatus->setText("● 设备已连接 (WiFi TCP)");
    m_lblStatus->setStyleSheet("color:green; font-weight:bold;");
    m_lblBle->setText("BLE: ESP32-S3 广播中");
    m_lblBle->setStyleSheet("color:#8e44ad; font-size:11px;");
    m_log->append("[系统] ESP32-S3 已连接 (WiFi)");
}

void MainWindow::onClientDisconnected()
{
    m_lblStatus->setText("● 设备已断开，等待重连...");
    m_lblStatus->setStyleSheet("color:red; font-weight:bold;");
    m_lblBle->setText("BLE: 未连接");
    m_log->append("[系统] 设备断开，TCP服务器继续监听...");
}
