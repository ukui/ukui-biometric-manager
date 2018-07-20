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
    void on_treeView_doubleClicked(const QModelIndex &);

/* Normal functions */
private:
	void showDeviceInfo();
    void showFeatures();
	bool deviceIsAvailable();
	void setModel();
	void updateWidgetStatus();
	void updateButtonUsefulness();
    FeatureInfo *createNewFeatureInfo();
    QString inputFeatureName(bool isNew);

public:
    void setDeviceAvailable(int deviceAvailable);
    int featuresCount();

/* DBus */
private slots:
    void showFeaturesCallback(QDBusMessage callbackReply);
	void errorCallback(QDBusError error);

/* Members */
private:
	Ui::ContentPane *ui;
	/* 用于和远端 DBus 对象交互的代理接口 */
    QDBusInterface *serviceInterface;
	DeviceInfo *deviceInfo;
    int currentUid;
    TreeModel *dataModel;
    int freeIndex; /* 录入时所用的空闲的特征 index */
    QString indexName; /* 录入时用户输入的特征名称 */
	/* 进度提示弹框 */
	PromptDialog *promptDialog;
	QString promptDialogGIF;
};

#endif // CONTENTPANE_H
