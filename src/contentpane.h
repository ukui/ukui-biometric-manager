#ifndef CONTENTPANE_H
#define CONTENTPANE_H

#include <QWidget>
#include <QStandardItemModel>
#include "customtype.h"
#include "biometric_interface.h"
#include "promptdialog.h"
#include "treemodel.h"

namespace Ui {
class ContentPane;
}

class ContentPane : public QWidget
{
	Q_OBJECT

public:
    explicit ContentPane(int uid, DeviceInfo *deviceInfo, QWidget *parent = 0);
	~ContentPane();

signals:
    void changeDeviceStatus(DeviceInfo *deviceInfo);

/* Qt Slots */
private slots:
	void on_btnEnroll_clicked();
	void on_btnDelete_clicked();
	void on_btnVerify_clicked();
	void on_btnSearch_clicked();
    void on_btnClean_clicked();
    void on_btnStatus_clicked();
    void on_btnDefault_clicked();

/* Normal functions */
private:
	void showDeviceInfo();
    void showFeatures();
	void setPromptDialogGIF();
	bool deviceIsAvailable();
	void setModel();
	void updateWidgetStatus();
	void updateButtonUsefulness();
    FeatureInfo *createNewFeatureInfo();
public:
    void setDeviceAvailable(int deviceAvailable);
    int featuresCount();

/* DBus */
private slots:
    void showFeaturesCallback(QDBusMessage callbackReply);
	void enrollCallback(QDBusMessage callbackReply);
	void errorCallback(QDBusError error);
	void cancelOperation(); /* 普通 Qt SLOT，不是DBus回调，用于接收弹窗按钮事件 */
	void cancelCallback(QDBusMessage callbackReply);
	void verifyCallback(QDBusMessage callbackReply);
	void searchCallback(QDBusMessage callbackReply);
	void setOperationMsg(int deviceID, int statusType); /* 普通 Qt SLOT，被 D-Bus 信号 StatusChanged 触发 */

/* Members */
private:
	Ui::ContentPane *ui;
	/* 用于和远端 DBus 对象交互的代理接口 */
    QDBusInterface *serviceInterface;
	DeviceInfo *deviceInfo;
	int selectedUid;
    TreeModel *dataModel;
    int freeIndex; /* 录入时所用的空闲的特征 index */
    QString indexName; /* 录入时用户输入的特征名称 */
	/* 进度提示弹框 */
	PromptDialog *promptDialog;
	QString promptDialogGIF;
    QStringList bioTypeText;
    bool isEnrolling;
};

#endif // CONTENTPANE_H
