#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "biometric_interface.h"
#include "customtype.h"
#include "toggleswitch.h"
#include <QTableWidgetItem>
#include "contentpane.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QString usernameFromCmd, QWidget *parent = 0);
	~MainWindow();

/* Qt slots */
private slots:
	void on_comboBoxUsername_currentIndexChanged(int index);
	void manageDeviceStatus(bool toState);
	void manageBioAuthStatus(bool toState);

/* Normal functions */
private:
    void checkServiceExist();
	void checkAPICompatibility();
	void prettify();
	void getDeviceInfo();
    void addContentPane(DeviceInfo *deviceInfo);
	void initialize();
	void enableBiometricTabs();
	void disableBiometricTabs();
	void initBiometricPage();
	void initDashboardDeviceSection();
	void initDashboardBioAuthSection();
	QIcon *getUserAvatar(QString username);
	void showUserList();
	void setDefaultUser();

/* Signals */
signals:
	void selectedUserChanged(int uid);

/* Members */
private:
	Ui::MainWindow *ui;
	/* 用于和远端 DBus 对象交互的代理接口 */
	cn::kylinos::Biometric *biometricInterface;
	int deviceCount;
    QList<DeviceInfo*> deviceInfoList;
	QMap<QString, ContentPane *> contentPaneMap;
	/* 通过命令行参数传入的用户名 */
	QString usernameFromCmd;
};

#endif // MAINWINDOW_H
