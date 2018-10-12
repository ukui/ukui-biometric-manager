QT       += core gui dbus
CONFIG   += c++11 link_pkgconfig
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = polkit-ukui-authentication-agent-1
TEMPLATE = app

CONFIG += debug

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0

DEFINES += INSTALL_PATH=${UKUI_BIOMETRIC}/ukui-polkit-agent \
           UKUI_BIOMETRIC=${UKUI_BIOMETRIC}

INCLUDEPATH +=  $$PWD/../common/ \
                $$PWD/../bioauth/include/

VPATH += $$PWD/../common/

LIBS +=  -lpolkit-qt5-core-1 \
        -L$$PWD/../bioauth -lbioauth


PKGCONFIG += polkit-qt5-agent-1

HEADERS += \
    src/mainwindow.h \
    src/PolkitListener.h \
    src/sessionmanager.h

FORMS += \
    src/mainwindow.ui

SOURCES += \
    src/PolkitAgent.cpp \
    src/mainwindow.cpp \
    src/PolkitListener.cpp \
    src/sessionmanager.cpp \
    generic.cpp \

RESOURCES += \
    assets.qrc

DISTFILES += \
    src/main.qss


TRANSLATIONS += i18n_ts/zh_CN.ts

system(lrelease i18n_ts/*.ts)

qm_file.files = i18n_ts/*.qm
qm_file.path = /$${UKUI_BIOMETRIC}/ukui-polkit-agent/i18n_qm/

desktop_file.files = data/*.desktop
desktop_file.path = /etc/xdg/autostart/

target.path = /${LIB_PATH}/ukui-polkit

INSTALLS += qm_file target desktop_file
