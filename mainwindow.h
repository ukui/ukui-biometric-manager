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
	void on_btnAdd_clicked();

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
	void dbusCallback(QDBusMessage callbackReply);
	void errorCallback(QDBusError error);
	void setOperationMsg();
	void cancelOperation(); /* 普通 SLOT，不是DBus回调 */
	void cancelCallback(QDBusMessage callbackReply);
	void showBiometricsCallback(QDBusMessage callbackReply);

private:
	Ui::MainWindow *ui;
	/* 用于和远端 DBus 对象交互的代理接口 */
	cn::kylinos::Biometric *biometricInterface;
	int deviceCount;
	QMap<enum BioType,DeviceInfo *> deviceInfoMap;
	enum BioType currentBiotype;
	int currentUid;
	/* TreeView 影射表 */
	QMap<enum BioType, QTreeView *> treeViewMap;
	/* 数据模型影射表 */
	QMap<enum BioType, QStandardItemModel *> dataModelMap;
	/* 各个设备占用的特征index */
	QMap<enum BioType, QList<int> *> biometricIndexMap;
	int freeIndexPos; /* 录入时所用的特征 index 在追踪列表中的下标 */
	QString indexName; /* 录入时用户输入的特征名称 */
	/* 进度提示弹框 */
	PromptDialog *promptDialog;
	QTimer *timer;
};

#endif // MAINWINDOW_H
