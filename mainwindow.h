#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
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

private:
	void getDeviceInfoList();
	bool deviceAvailable(enum BIOTYPE biotype);

private:
	Ui::MainWindow *ui;
	/* 用于和远端 DBus 对象交互的代理接口 */
	cn::kylinos::Biometric *biometricInterface;
	int deviceCount;
	QList<DeviceInfo *> deviceInfoList;
};

#endif // MAINWINDOW_H
