#-------------------------------------------------
#
# Project created by QtCreator 2017-07-10T09:27:42
#
#-------------------------------------------------

TRANSLATIONS = i18n/zh_CN.ts
QT       += core gui dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = BiometricManager
TEMPLATE = app
DBUS_INTERFACES += cn.kylinos.Biometric.xml


SOURCES += main.cpp\
    mainwindow.cpp \
    customtype.cpp \
    promptdialog.cpp

HEADERS  += mainwindow.h \
    customtype.h \
    promptdialog.h

FORMS    += mainwindow.ui \
    promptdialog.ui

RESOURCES += \
    assets.qrc
