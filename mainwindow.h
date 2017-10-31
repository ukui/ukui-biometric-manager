#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QProgressDialog>
#include <QTreeView>
#include "biometric_interface.h"
#include "customtype.h"
#include "promptdialog.h"

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
	void on_comboBoxUname_currentIndexChanged(int index);
	void on_btnEnroll_clicked();
	void on_btnDelete_clicked();
	void on_btnDrop_clicked();
	void on_btnVerify_clicked();
	void on_btnSearch_clicked();

private:
	void getDeviceInfo();
	bool deviceIsEnable(enum BioType biotype);
	void showBiometrics();
	void showUserList();
	void trackAllBiometricIndex();
	int findFreeBiometricIndex();
	void setWidgetsEnabled(bool status);
	void setModel();

private slots:
	void enrollCallback(QDBusMessage callbackReply);
	void errorCallback(QDBusError error);
	void cancelOperation(); /* 普通 Qt SLOT，不是DBus回调，用于接收弹窗按钮事件 */
	void cancelCallback(QDBusMessage callbackReply);
	void showBiometricsCallback(QDBusMessage callbackReply);
	void verifyCallback(QDBusMessage callbackReply);
	void searchCallback(QDBusMessage callbackReply);
	void setOperationMsg(int driverID, int statusType); /* 普通 Qt SLOT，被 D-Bus 信号 StatusChanged 触发 */

private:
	Ui::MainWindow *ui;
	/* 用于和远端 DBus 对象交互的代理接口 */
	cn::kylinos::Biometric *biometricInterface;
	int deviceCount;
	QMap<enum BioType,DeviceInfo *> deviceInfoMap;
	enum BioType currentBiotype;
	int currentUid;
	/* TreeView 映射表 */
	QMap<enum BioType, QTreeView *> treeViewMap;
	/* 数据模型映射表 */
	QMap<enum BioType, QStandardItemModel *> dataModelMap;
	/* 各个设备占用的特征index */
	QMap<enum BioType, QList<int> *> biometricIndexMap;
	int freeIndex; /* 录入时所用的空闲的特征 index */
	QString indexName; /* 录入时用户输入的特征名称 */
	/* 进度提示弹框 */
	PromptDialog *promptDialog;
	/* 存储设备类型和 GIF 的映射关系 */
	QMap<enum BioType, QString> gifMap;
};

#endif // MAINWINDOW_H
