#pragma once
#include <QDialog>
#include <QTableWidget>
#include <QDateTimeEdit>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

class HistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit HistoryDialog(const QString &csvPath, QWidget *parent = nullptr);

private slots:
    void onQuery();

private:
    void updateChart();

    QString          m_csvPath;
    QDateTimeEdit   *m_dtFrom;
    QDateTimeEdit   *m_dtTo;
    QTableWidget    *m_table;
    QChartView      *m_chartView;
    QLineSeries     *m_serTemp;
    QLineSeries     *m_serHumi;
    QLineSeries     *m_serLight;
    QLineSeries     *m_serPres;
};
