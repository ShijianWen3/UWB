#-------------------------------------------------
# Project created by QtCreator 2014-05-28T11:50:00
#-------------------------------------------------


cache()

QT       += core gui network xml serialport quickwidgets
QT       += network
QT       += core gui charts
QT       += datavisualization


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# TARGET = HR-RTLS_PC
TARGET = UWB
TEMPLATE = app

# 移除或注释掉这行如果没有Info.plist文件
QMAKE_INFO_PLIST = Info.plist

INCLUDEPATH += models network views util tools

INCLUDEPATH += $$PWD/armadillo-3.930.0/include
# INCLUDEPATH += $$PWD/armadillo-15.0.1/include


LIBS += -L$$PWD/armadillo-3.930.0/lib/ -lblas_win32_MT
LIBS += -L$$PWD/armadillo-3.930.0/lib/ -llapack_win32_MT

# LIBS += -L$$PWD/armadillo-15.0.1/lib/ -llibopenblas


SOURCES += main.cpp \
    RTLSDisplayApplication.cpp \
    network/network_udp.cpp \
    views/mainwindow.cpp \
    network/RTLSClient.cpp \
    views/GraphicsView.cpp \
    views/GraphicsWidget.cpp \
    views/ViewSettingsWidget.cpp \
    views/MinimapView.cpp \
    views/connectionwidget.cpp \
    models/ViewSettings.cpp \
    tools/OriginTool.cpp \
    tools/RubberBandTool.cpp \
    tools/ScaleTool.cpp \
    util/QPropertyModel.cpp \
    network/SerialConnection.cpp \
    tools/trilateration.cpp

HEADERS += \
    RTLSDisplayApplication.h \
    network/network_udp.h \
    views/mainwindow.h \
    network/RTLSClient.h \
    views/GraphicsView.h \
    views/GraphicsWidget.h \
    views/ViewSettingsWidget.h \
    views/MinimapView.h \
    views/connectionwidget.h \
    models/ViewSettings.h \
    tools/AbstractTool.h \
    tools/OriginTool.h \
    tools/RubberBandTool.h \
    tools/ScaleTool.h \
    util/QPropertyModel.h \
    network/SerialConnection.h \
    tools/trilateration.h

FORMS += \
    views/mainwindow.ui \
    views/GraphicsWidget.ui \
    views/ViewSettingsWidget.ui \
    views/connectionwidget.ui

#生成的ui_*.h的文件存储路径(..相对于的是生成的.exe路径)
UI_DIR = ..\\..\\ui

RESOURCES += \
    res/lanague.qrc\
    res/resources.qrc

TRANSLATIONS += \
    language_cn.ts\
    language_en.ts

# 检查这个文件是否存在
RC_FILE = logo.rc


include ($$PWD/gsl/gsl.pri)

DISTFILES += \
    .gitignore


