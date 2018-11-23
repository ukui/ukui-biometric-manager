#include "servicemanager.h"
#include <QDebug>
#include "customtype.h"
#include "messagedialog.h"

ServiceManager *ServiceManager::instance_ = nullptr;

ServiceManager::ServiceManager(QObject *parent)
    : QObject(parent),
      dbusService(nullptr),
      bioService(nullptr)
{
    init();
}

void ServiceManager::init()
{
    if(!dbusService)
    {
        dbusService = new QDBusInterface(FD_DBUS_SERVICE,
                                         FD_DBUS_PATH,
                                         FD_DBUS_INTERFACE,
                                         QDBusConnection::systemBus());
        connect(dbusService, SIGNAL(NameOwnerChanged(QString, QString, QString)),
                this, SLOT(onDBusNameOwnerChanged(QString,QString,QString)));
    }
}

ServiceManager *ServiceManager::instance()
{
    if(!instance_)
    {
        instance_ = new ServiceManager;
    }
    return instance_;
}

bool ServiceManager::connectToService()
{
    if(!bioService)
    {
        bioService = new QDBusInterface(DBUS_SERVICE,
                                        DBUS_PATH,
                                        DBUS_INTERFACE,
                                        QDBusConnection::systemBus());
    }
    return bioService->isValid();
}

void ServiceManager::onDBusNameOwnerChanged(const QString &name,
                                            const QString &oldOwner,
                                            const QString &newOwner)
{
    if(name == DBUS_SERVICE)
    {
        qDebug() << "service status changed:"
                 << (newOwner.isEmpty() ? "inactivate" : "activate");
        Q_EMIT serviceStatusChanged(!newOwner.isEmpty());
    }
}

/*!
 * \brief checkServiceExist
 * 检查生物识别后台服务是否已启动
 */
bool ServiceManager::serviceExists()
{
    QDBusReply<bool> reply = dbusService->call("NameHasOwner", DBUS_SERVICE);
    if(!reply.isValid())
    {
        qDebug() << "check service exists error:" << reply.error();
        return false;
    }
    return reply.value();
}

/*!
 * \brief ServiceManager::apiCompatible
 * 检查API版本和服务的版本是否兼容
 */
bool ServiceManager::apiCompatible()
{
    if(!connectToService())
        return false;

    QDBusReply<int> reply = bioService->call("CheckAppApiVersion",
                                             APP_API_MAJOR,
                                             APP_API_MINOR,
                                             APP_API_FUNC);
    if(!reply.isValid())
    {
        qDebug() << "check api compatibility error: " << reply.error();
        return false;
    }
    return (reply.value() == 0);
}
