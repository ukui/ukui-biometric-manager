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

#include "sessionmanager.h"
#include <QApplication>
#include <QDBusReply>
#include <QDBusObjectPath>
#include <QDebug>

#define SM_DBUS_SERVICE "org.gnome.SessionManager"
#define SM_DBUS_PATH "/org/gnome/SessionManager"
#define SM_DBUS_INTERFACE "org.gnome.SessionManager"
#define SM_DBUS_CLIENT_INTERFACE "org.gnome.SessionManager.ClientPrivate"


SessionManager::SessionManager(QObject *parent) : QObject(parent)
{
    smInterface = new QDBusInterface(SM_DBUS_SERVICE,
                             SM_DBUS_PATH,
                             SM_DBUS_INTERFACE,
                             QDBusConnection::sessionBus());
    QString appId("polkit-ukui-authentication-agent-1.desktop");
    QString clientStartupId(qgetenv("DESKTOP_AUTOSTART_ID"));

    QDBusReply<QDBusObjectPath> reply = smInterface->call("RegisterClient",
                                                       appId, clientStartupId);

    if(!reply.isValid()) {
        qWarning() << "Register Client to gnome session failed";
    }
    clientId = reply.value().path();

    qDebug() << "Register Client to gnome session: " << reply.value().path();

    clientInterface = new QDBusInterface(SM_DBUS_SERVICE,
                                   clientId,
                                   SM_DBUS_CLIENT_INTERFACE,
                                   QDBusConnection::sessionBus());
    qDebug() << clientInterface->isValid();

    QDBusConnection conn = clientInterface->connection();
    conn.connect(SM_DBUS_SERVICE, clientId, SM_DBUS_CLIENT_INTERFACE,
                 "Stop", this, SLOT(onStop()));
    conn.connect(SM_DBUS_SERVICE, clientId, SM_DBUS_CLIENT_INTERFACE,
                 "EndSession", this, SLOT(onEndSession(unsigned int)));
    conn.connect(SM_DBUS_SERVICE, clientId, SM_DBUS_CLIENT_INTERFACE,
                 "QueryEndSession", this, SLOT(onQueryEndSession(unsigned int)));

}

void SessionManager::onStop()
{
    QApplication::quit();
}

void SessionManager::onEndSession(unsigned int flag)
{
    endSessionResponse();
}

void SessionManager::onQueryEndSession(unsigned int flag)
{
    endSessionResponse();
    QApplication::quit();
}

void SessionManager::endSessionResponse()
{
    QDBusMessage msg = clientInterface->call("EndSessionResponse", true, "");
    if(msg.type() == QDBusMessage::ErrorMessage)
        qDebug() << "Failed to call EndSessionResponse " << msg.errorMessage();
}
