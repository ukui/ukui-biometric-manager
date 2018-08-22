#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableWidgetItem>
#include "biometric_interface.h"
#include "customtype.h"
#include "contentpane.h"

namespace Ui {
class MainWindow;
}
class QLabel;
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

    void onDeviceStatusClicked();
    bool changeDeviceStatus(DeviceInfo *deviceInfo);
    void on_btnMenu_clicked();

private:
    void checkServiceExist();
	void checkAPICompatibility();
	void prettify();
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



/* Members */
private:
	Ui::MainWindow *ui;
    QLabel  *lblStatus;
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
};

#endif // MAINWINDOW_H
