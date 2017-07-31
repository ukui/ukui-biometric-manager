#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include "biometric_interface.h"
#include "devicespec.h"
#include "customtype.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_tabWidget_currentChanged(int index);

	void on_comboBoxUname_activated(int index);

private:
	void getDeviceInfoList();
	bool deviceIsEnable();
	void showBiometrics();
	void getUserList();

private:
	Ui::MainWindow *ui;
	/* 用于和远端 DBus 对象交互的代理接口 */
	cn::kylinos::Biometric *biometricInterface;
	int deviceCount;
	QMap<enum BIOTYPE,DeviceInfo *> deviceInfoMap;
	enum BIOTYPE currentBiotype;
	int currentUid;
	QStandardItemModel *modelFingervein;
	/* 记录标签页是否是第一次被展示 */
	bool pageFirstShow[3];
};

#endif // MAINWINDOW_H
