#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>

struct AppSettings {
    quint16 port      = 8888;
    float   tempMax   = 35.0f;
    float   humiMax   = 80.0f;
    int     lightMin  = 20;
    float   presMin   = 990.0f;
    float   presMax   = 1030.0f;
};

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const AppSettings &s, QWidget *parent = nullptr);
    AppSettings settings() const;

private:
    QSpinBox        *m_port;
    QDoubleSpinBox  *m_tempMax;
    QDoubleSpinBox  *m_humiMax;
    QSpinBox        *m_lightMin;
    QDoubleSpinBox  *m_presMin;
    QDoubleSpinBox  *m_presMax;
};
