/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
**/

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
