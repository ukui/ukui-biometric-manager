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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include <QCheckBox>
#include "customtype.h"
#include "contentpane.h"

namespace Ui {
class MainWindow;
}
class QLabel;
class AboutDialog;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QString usernameFromCmd, QWidget *parent = 0);
	~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

/* Qt slots */
private slots:
    void on_btnDashBoard_clicked();
    void on_btnFingerPrint_clicked();
    void on_btnFingerVein_clicked();
    void on_btnIris_clicked();
    void on_btnVoicePrint_clicked();
    void on_btnStatus_clicked();
    void on_listWidgetDevicesType_currentRowChanged(int);
    void on_tableWidgetDevices_cellDoubleClicked(int row, int column);

    void onDriverStatusClicked();
    void onDefaultDeviceChanged(bool checked);
    bool changeDeviceStatus(DeviceInfo *deviceInfo);
    void onUSBDeviceHotPlug(int, int, int);

public slots:
    void onServiceStatusChanged(bool activate);

private:
	void prettify();
    void initSysMenu();
	void getDeviceInfo();
    void addContentPane(DeviceInfo *deviceInfo);
	void initialize();
    void initDeviceTypeList();
	void initBiometricPage();
	void initDashboardBioAuthSection();
    QPixmap *getUserAvatar(QString username);
    void setCurrentUser();
    void changeBtnColor(QPushButton *btn);
    void setVerificationStatus(bool status);
    int bioTypeToIndex(int type);
    bool restartService();
    void updateDevice();
    void updateDeviceListWidget(int biotype);
    void setDeviceStatus(QTableWidgetItem *item, bool connected);




/* Members */
private:
	Ui::MainWindow *ui;
    QLabel  *lblStatus;
    //由于互斥的QButtonGroup会保证至少有一个被选中，所以这里不使用它
    QList<QCheckBox*> btnGroup;
	/* 用于和远端 DBus 对象交互的代理接口 */
    QDBusInterface *serviceInterface;
	int deviceCount;
    QMap<int, QList<DeviceInfo *>> deviceInfosMap;
	QMap<QString, ContentPane *> contentPaneMap;
	/* 通过命令行参数传入的用户名 */
    QString username;
    bool verificationStatus;    //生物识别开关状态

    //for window move
    QPoint dragPos;
    bool dragWindow;

    QMenu *menu;
    AboutDialog *aboutDlg;

    /* 服务被关闭时提示 */
    QLabel *lblPrompt;
};

#endif // MAINWINDOW_H
