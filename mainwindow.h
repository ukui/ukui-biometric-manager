#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "biometric_interface.h"
#include "customtype.h"
#include "toggleswitch.h"
#include <QTableWidgetItem>
#include <contentpane.h>

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
	void restartService();
	void manageDriverStatus(bool toState);
	void manageBioAuthStatus(bool toState);

/* Normal functions */
private:
	void checkAPICompatibility();
	void prettify();
	void getDeviceInfo();
	void addContentPane(QString driverName);
	void initialize();
	void enableBiometricTabs();
	void disableBiometricTabs();
	void biometricPageInit();
	QString mapReadableDeviceName(QString driverName);
	void dashboardDriverSection();
	void dashboardBioAuthSection();
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
	int driverCount;
	QMap<QString, DeviceInfo *> deviceInfoMap;
	QMap<QString, ContentPane *> contentPaneMap;
	/* 通过命令行参数传入的用户名 */
	QString usernameFromCmd;
};

#endif // MAINWINDOW_H
