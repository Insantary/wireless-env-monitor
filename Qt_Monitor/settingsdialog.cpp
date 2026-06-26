#include "settingsdialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>

SettingsDialog::SettingsDialog(const AppSettings &s, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("系统参数配置");
    setMinimumWidth(320);

    // 端口
    m_port = new QSpinBox();
    m_port->setRange(1024, 65535);
    m_port->setValue(s.port);

    // 报警阈值
    m_tempMax = new QDoubleSpinBox();
    m_tempMax->setRange(20, 60); m_tempMax->setSuffix(" °C");
    m_tempMax->setValue(s.tempMax);

    m_humiMax = new QDoubleSpinBox();
    m_humiMax->setRange(30, 100); m_humiMax->setSuffix(" %");
    m_humiMax->setValue(s.humiMax);

    m_lightMin = new QSpinBox();
    m_lightMin->setRange(0, 100); m_lightMin->setSuffix(" %");
    m_lightMin->setValue(s.lightMin);

    m_presMin = new QDoubleSpinBox();
    m_presMin->setRange(900, 1050); m_presMin->setSuffix(" hPa");
    m_presMin->setValue(s.presMin);

    m_presMax = new QDoubleSpinBox();
    m_presMax->setRange(900, 1050); m_presMax->setSuffix(" hPa");
    m_presMax->setValue(s.presMax);

    auto *grpNet = new QGroupBox("网络设置");
    auto *fNet   = new QFormLayout(grpNet);
    fNet->addRow("监听端口：", m_port);

    auto *grpAlm = new QGroupBox("报警阈值");
    auto *fAlm   = new QFormLayout(grpAlm);
    fAlm->addRow("温度上限：", m_tempMax);
    fAlm->addRow("湿度上限：", m_humiMax);
    fAlm->addRow("光照下限：", m_lightMin);
    fAlm->addRow("气压下限：", m_presMin);
    fAlm->addRow("气压上限：", m_presMax);

    auto *note = new QLabel("注：端口修改后重启软件生效");
    note->setStyleSheet("color:gray; font-size:11px;");

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(grpNet);
    layout->addWidget(grpAlm);
    layout->addWidget(note);
    layout->addWidget(btns);
}

AppSettings SettingsDialog::settings() const {
    AppSettings s;
    s.port     = (quint16)m_port->value();
    s.tempMax  = (float)m_tempMax->value();
    s.humiMax  = (float)m_humiMax->value();
    s.lightMin = m_lightMin->value();
    s.presMin  = (float)m_presMin->value();
    s.presMax  = (float)m_presMax->value();
    return s;
}
