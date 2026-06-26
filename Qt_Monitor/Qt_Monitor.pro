QT += core gui widgets network charts

CONFIG += c++17

TARGET   = Qt_Monitor
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    tcpserver.cpp \
    settingsdialog.cpp \
    historydialog.cpp

HEADERS += \
    mainwindow.h \
    tcpserver.h \
    settingsdialog.h \
    historydialog.h
