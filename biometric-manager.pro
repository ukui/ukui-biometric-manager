#-------------------------------------------------
#
# Project created by QtCreator 2017-07-10T09:27:42
#
#-------------------------------------------------

TRANSLATIONS = i18n_ts/zh_CN.ts
QT       += core gui dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

PREFIX = /usr/share/biometric-manager

TARGET = biometric-manager
TEMPLATE = app
DBUS_INTERFACES += data/cn.kylinos.Biometric.xml


SOURCES += src/main.cpp\
    src/mainwindow.cpp \
    src/customtype.cpp \
    src/promptdialog.cpp \
    src/contentpane.cpp \
    src/toggleswitch.cpp

HEADERS  += src/mainwindow.h \
    src/customtype.h \
    src/promptdialog.h \
    src/contentpane.h \
    src/toggleswitch.h

FORMS    += src/mainwindow.ui \
    src/promptdialog.ui \
    src/contentpane.ui \

RESOURCES += \
    assets.qrc

TRANSLATIONS += i18n_ts/zh_CN.ts

system("lrelease i18n_ts/zh_CN.ts i18n_ts/zh_CN.qm")

qm_file.files = i18n_ts/*.qm
qm_file.path = $${PREFIX}/i18n_qm/

target.path = /usr/bin/

INSTALLS += target qm_file
