#include "configuration.h"
#include <QSettings>
#include <QDir>
#include <QDebug>

QString Configuration::configFile = QDir::homePath() +
        "/.biometric_auth/ukui_biometric.conf";

Configuration* Configuration::instance_ = new Configuration;

Configuration::Configuration(QObject *parent) : QObject(parent)
{ 

}

Configuration *Configuration::instance()
{
    return instance_;
}

QString Configuration::getDefaultDevice()
{
    QSettings settings(configFile, QSettings::IniFormat);

    return settings.value("DefaultDevice").toString();
}

void Configuration::setDefaultDevice(const QString &deviceName)
{
    qDebug() << deviceName;
    QSettings settings(configFile, QSettings::IniFormat);

    settings.setValue("DefaultDevice", deviceName);
    settings.sync();

    emit defaultDeviceChanged(deviceName);
}
