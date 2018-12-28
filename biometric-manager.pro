#-------------------------------------------------
#
# Project created by QtCreator 2017-07-10T09:27:42
#
#-------------------------------------------------

QT       += core gui dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

DEFINES += APP_API_MAJOR=0  \
            APP_API_MINOR=11    \
            APP_API_FUNC=0

PREFIX = /usr/share/biometric-manager

TARGET = biometric-manager
TEMPLATE = app


SOURCES += src/main.cpp\
    src/mainwindow.cpp \
    src/customtype.cpp \
    src/promptdialog.cpp \
    src/contentpane.cpp \
    src/treeitem.cpp \
    src/treemodel.cpp \
    src/inputdialog.cpp \
    src/messagedialog.cpp \
    src/aboutdialog.cpp \
    src/configuration.cpp \
    src/servicemanager.cpp


HEADERS  += src/mainwindow.h \
    src/customtype.h \
    src/promptdialog.h \
    src/contentpane.h \
    src/treeitem.h \
    src/treemodel.h \
    src/inputdialog.h \
    src/messagedialog.h \
    src/aboutdialog.h \
    src/configuration.h \
    src/servicemanager.h


FORMS    += src/mainwindow.ui \
    src/promptdialog.ui \
    src/contentpane.ui \
    src/inputdialog.ui \
    src/messagedialog.ui \
    src/aboutdialog.ui


RESOURCES += \
    assets.qrc

TRANSLATIONS += i18n_ts/zh_CN.ts \
                i18n_ts/fr.ts \
                i18n_ts/pt.ts \
                i18n_ts/ru.ts \
                i18n_ts/es.ts

system("lrelease i18n_ts/*.ts")

qm_file.files = i18n_ts/*.qm
qm_file.path = $${PREFIX}/i18n_qm/

target.path = /usr/bin/

ICON.files = assets/biometric-manager.png
ICON.path = /usr/share/pixmaps/

desktop.files = data/biometric-manager.desktop
desktop.path = /usr/share/applications/

INSTALLS += target qm_file ICON desktop
