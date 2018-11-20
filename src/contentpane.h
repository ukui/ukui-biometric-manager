/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
**/
#ifndef CONTENTPANE_H
#define CONTENTPANE_H

#include <QWidget>
#include <QStandardItemModel>
#include "customtype.h"
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
    void on_treeView_doubleClicked(const QModelIndex &);

/* Normal functions */
private:
	void showDeviceInfo();
	bool deviceIsAvailable();
	void setModel();
	void updateWidgetStatus();
	void updateButtonUsefulness();
    FeatureInfo *createNewFeatureInfo();
    QString inputFeatureName(bool isNew);
    QString getErrorMessage(int, int);
    bool confirmDelete(bool all);

    enum{DELETE, CLEAN, RENAME};

public:
    void setDeviceAvailable(int deviceAvailable);
    void setDeviceInfo(DeviceInfo *deviceInfo);
    int featuresCount();
    void showFeatures();

/* DBus */
private slots:
    void showFeaturesCallback(QDBusMessage callbackReply);
	void errorCallback(QDBusError error);

/* Members */
    void on_cbDefault_clicked(bool checked);

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
