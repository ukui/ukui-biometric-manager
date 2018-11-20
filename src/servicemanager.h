#ifndef SERVICEMANAGER_H
#define SERVICEMANAGER_H

#include <QObject>
#include <QDBusInterface>

class ServiceManager : public QObject
{
    Q_OBJECT
public:
    static ServiceManager *instance();
    bool serviceExists();
    bool apiCompatible();

private:
    explicit ServiceManager(QObject *parent = nullptr);
    void init();
    bool connectToService();

signals:
    void serviceStatusChanged(bool activate);

public slots:
    void onDBusNameOwnerChanged(const QString &name,
                                const QString &oldOwner,
                                const QString &newOwner);

private:
    static ServiceManager   *instance_;
    QDBusInterface          *dbusService;
    QDBusInterface          *bioService;
    bool                    serviceStatus;
};

#endif // SERVICEMANAGER_H
