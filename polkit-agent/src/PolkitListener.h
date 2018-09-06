#ifndef POLKITLISTENER_H
#define POLKITLISTENER_H

#include <PolkitQt1/Agent/Listener>
#include <QWeakPointer>

#include "mainwindow.h"

using namespace PolkitQt1::Agent;

class PolkitListener : public Listener
{
	Q_OBJECT
public:
	PolkitListener(QObject *parent = 0);
	virtual ~PolkitListener();

public slots:
	void initiateAuthentication(const QString &actionId,
				    const QString &message,
				    const QString &iconName,
				    const PolkitQt1::Details &details,
				    const QString &cookie,
				    const PolkitQt1::Identity::List &identities,
				    PolkitQt1::Agent::AsyncResult *result);
	bool initiateAuthenticationFinish();
	void cancelAuthentication();
	void finishObtainPrivilege();


private:
	bool gainedAuthorization;
	bool wasCancelled;
    bool wasSwitchToBiometric;
    bool inProgress;
	int numTries;
	QWeakPointer<Session> session;
	PolkitQt1::Identity::List identities;
	PolkitQt1::Identity currentIdentity;
	PolkitQt1::Agent::AsyncResult *result;
	QString cookie;
    MainWindow *mainWindow;

private slots:

    void startAuthentication();
    void onShowPrompt(const QString &prompt, bool echo);
    void onShowError(const QString &text);
    void onShowInfo(const QString &text);
    void onResponse(const QString &text);
    void onAuthCompleted(bool);
};
#endif /* POLKITLISTENER_H */
