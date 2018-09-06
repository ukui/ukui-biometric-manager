#ifndef BIOAUTH_H
#define BIOAUTH_H

#include <QObject>
#include "biotypes.h"

class QDBusInterface;

/*!
 * \brief The BioAuth class
 * 负责真正的生物识别操作，通过startAuth开始认证，
 * 认证完成后会发出携带结果的authComplete信号
 */
class BioAuth : public QObject
{
    Q_OBJECT
public:
    explicit BioAuth(qint32 uid, const DeviceInfo &deviceInfo, QObject *parent = nullptr);
    ~BioAuth();
    void startAuth();
    void stopAuth();
    bool isAuthenticating();

signals:
    void authComplete(uid_t uid, bool result);
    void notify(const QString &message);

private slots:
    void onIdentityComplete(QDBusPendingCallWatcher *watcher);
    void onStatusChanged(int deviceId, int statusType);

private:
    QDBusInterface      *serviceInterface;

    qint32              uid;
    DeviceInfo          deviceInfo;
    bool isInAuthentication;
};

#endif // BIOAUTH_H
