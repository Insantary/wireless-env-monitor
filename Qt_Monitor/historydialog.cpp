#include "historydialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QFile>
#include <QTextStream>
#include <QLabel>
#include <QtCharts/QChart>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QMessageBox>
#include <QSplitter>

HistoryDialog::HistoryDialog(const QString &csvPath, QWidget *parent)
    : QDialog(parent), m_csvPath(csvPath)
{
    setWindowTitle("历史数据查询");
    resize(900, 600);

    // 时间选择器
    m_dtFrom = new QDateTimeEdit(QDateTime::currentDateTime().addDays(-1));
    m_dtFrom->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_dtFrom->setCalendarPopup(true);

    m_dtTo = new QDateTimeEdit(QDateTime::currentDateTime());
    m_dtTo->setDisplayFormat("yyyy-MM-dd HH:mm:ss");
    m_dtTo->setCalendarPopup(true);

    auto *btnQuery = new QPushButton("查询");
    connect(btnQuery, &QPushButton::clicked, this, &HistoryDialog::onQuery);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(new QLabel("开始时间："));
    topLayout->addWidget(m_dtFrom);
    topLayout->addWidget(new QLabel("结束时间："));
    topLayout->addWidget(m_dtTo);
    topLayout->addWidget(btnQuery);
    topLayout->addStretch();

    // 数据表格
    m_table = new QTableWidget();
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({"时间", "温度(°C)", "湿度(%)", "光照(%)", "气压(hPa)"});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setMinimumHeight(200);

    // 历史曲线图
    m_serTemp  = new QLineSeries(); m_serTemp->setName("温度");  m_serTemp->setColor(Qt::red);
    m_serHumi  = new QLineSeries(); m_serHumi->setName("湿度");  m_serHumi->setColor(Qt::blue);
    m_serLight = new QLineSeries(); m_serLight->setName("光照"); m_serLight->setColor(QColor("#f39c12"));
    m_serPres  = new QLineSeries(); m_serPres->setName("气压");  m_serPres->setColor(Qt::green);

    QChart *chart = new QChart();
    chart->addSeries(m_serTemp);
    chart->addSeries(m_serHumi);
    chart->addSeries(m_serLight);
    chart->setTitle("历史数据趋势");
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

    auto *axX = new QDateTimeAxis();
    axX->setFormat("hh:mm:ss");
    axX->setTitleText("时间");
    chart->addAxis(axX, Qt::AlignBottom);
    m_serTemp->attachAxis(axX);
    m_serHumi->attachAxis(axX);
    m_serLight->attachAxis(axX);

    auto *axY = new QValueAxis();
    axY->setRange(0, 100);
    axY->setTitleText("数值");
    chart->addAxis(axY, Qt::AlignLeft);
    m_serTemp->attachAxis(axY);
    m_serHumi->attachAxis(axY);
    m_serLight->attachAxis(axY);

    m_chartView = new QChartView(chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(250);

    auto *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(m_table);
    splitter->addWidget(m_chartView);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(splitter);

    // 默认加载数据
    onQuery();
}

void HistoryDialog::onQuery() {
    QFile file(m_csvPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "错误", "无法打开数据文件：" + m_csvPath);
        return;
    }

    QDateTime from = m_dtFrom->dateTime();
    QDateTime to   = m_dtTo->dateTime();

    m_serTemp->clear();
    m_serHumi->clear();
    m_serLight->clear();
    m_serPres->clear();
    m_table->setRowCount(0);

    QTextStream in(&file);
    bool header = true;
    int row = 0;

    while (!in.atEnd()) {
        QString line = in.readLine();
        if (header) { header = false; continue; }

        QStringList parts = line.split(',');
        if (parts.size() < 4) continue;

        QDateTime dt = QDateTime::fromString(parts[0].trimmed(), "yyyy-MM-dd hh:mm:ss");
        if (!dt.isValid()) continue;
        if (dt < from || dt > to) continue;

        float temp  = parts[1].trimmed().toFloat();
        float humi  = parts[2].trimmed().toFloat();
        int   light = parts[3].trimmed().toInt();
        float pres  = parts.size() >= 5 ? parts[4].trimmed().toFloat() : 1013.25f;

        qint64 ms = dt.toMSecsSinceEpoch();
        m_serTemp->append(ms, temp);
        m_serHumi->append(ms, humi);
        m_serLight->append(ms, light);

        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(parts[0].trimmed()));
        m_table->setItem(row, 1, new QTableWidgetItem(QString::number(temp, 'f', 1)));
        m_table->setItem(row, 2, new QTableWidgetItem(QString::number(humi, 'f', 1)));
        m_table->setItem(row, 3, new QTableWidgetItem(QString::number(light)));
        m_table->setItem(row, 4, new QTableWidgetItem(QString::number(pres, 'f', 1)));
        row++;
    }
    file.close();

    if (row == 0) {
        m_table->insertRow(0);
        m_table->setItem(0, 0, new QTableWidgetItem("该时间段内无数据"));
        return;
    }

    // 更新坐标轴范围
    if (!m_serTemp->points().isEmpty()) {
        auto *chart = m_chartView->chart();
        auto axes = chart->axes(Qt::Horizontal);
        if (!axes.isEmpty()) {
            auto *axX = qobject_cast<QDateTimeAxis*>(axes.first());
            if (axX) {
                axX->setRange(from, to);
            }
        }
    }
}
