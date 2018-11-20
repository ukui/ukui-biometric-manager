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
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDBusArgument>
#include <QFile>
#include <QProcessEnvironment>
#include <QScrollBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QMenu>
#include <QCheckBox>
#include <QSettings>
#include <unistd.h>
#include <pwd.h>
#include "contentpane.h"
#include "customtype.h"
#include "messagedialog.h"
#include "aboutdialog.h"
#include "configuration.h"


#define ICON_SIZE 32

MainWindow::MainWindow(QString usernameFromCmd, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
    username(usernameFromCmd),
    verificationStatus(false),
    dragWindow(false),
    aboutDlg(nullptr)
{
	ui->setupUi(this);
	prettify();

    initialize();
    setWindowIcon(QIcon(":/images/assets/logo.png"));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton) {
        dragPos = event->globalPos() - pos();
        dragWindow = true;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if(dragWindow) {
        move(event->globalPos() - dragPos);
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent */*event*/)
{
    dragWindow = false;
}

void MainWindow::prettify()
{
    setWindowFlags(Qt::FramelessWindowHint);
	/* 设置窗口图标 */
    QApplication::setWindowIcon(QIcon(":/images/assets/icon.png"));
	/* 设置 CSS */
	QFile qssFile(":/css/assets/mainwindow.qss");
	qssFile.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(qssFile.readAll());
	this->setStyleSheet(styleSheet);
	qssFile.close();

	/* Set Icon for each tab on tabwidget */
    ui->btnDashBoard->setIcon(QIcon(":/images/assets/dashboard_default.png"));
    ui->btnFingerPrint->setIcon(QIcon(":/images/assets/fingerprint_default.png"));
    ui->btnFingerVein->setIcon(QIcon(":/images/assets/fingervein_default.png"));
    ui->btnIris->setIcon(QIcon(":/images/assets/iris_default.png"));
    ui->btnVoicePrint->setIcon(QIcon(":/images/assets/voiceprint_default.png"));
    /* Set logo on lblLogo */
    ui->lblLogo->setPixmap(QPixmap(":/images/assets/logo.png"));
    ui->btnMin->setIcon(QIcon(":/images/assets/min.png"));
    ui->btnClose->setIcon(QIcon(":/images/assets/close.png"));
    ui->btnMenu->setIcon(QIcon(":/images/assets/menu.png"));
}

QPixmap *MainWindow::getUserAvatar(QString username)
{
	QString iconPath;
	QDBusInterface userIface( "org.freedesktop.Accounts", "/org/freedesktop/Accounts",
		      "org.freedesktop.Accounts", QDBusConnection::systemBus());
	if (!userIface.isValid())
		qDebug() << "GUI:" << "userIface is invalid";
	QDBusReply<QDBusObjectPath> userReply = userIface.call("FindUserByName", username);
	if (!userReply.isValid()) {
		qDebug() << "GUI:" << "userReply is invalid";
		iconPath = "/usr/share/kylin-greeter/default_face.png";
	}
	QDBusInterface iconIface( "org.freedesktop.Accounts", userReply.value().path(),
			"org.freedesktop.DBus.Properties", QDBusConnection::systemBus());
	if (!iconIface.isValid())
		qDebug() << "GUI:" << "IconIface is invalid";
	QDBusReply<QDBusVariant> iconReply = iconIface.call("Get", "org.freedesktop.Accounts.User", "IconFile");
	if (!iconReply.isValid()) {
		qDebug() << "GUI:" << "iconReply is invalid";
		iconPath = "/usr/share/kylin-greeter/default_face.png";
	}
	iconPath = iconReply.value().variant().toString();
	if (access(qPrintable(iconPath), R_OK) != 0) /* No Access Permission */
		iconPath = "/usr/share/kylin-greeter/default_face.png";
    return new QPixmap(iconPath);
}

void MainWindow::setCurrentUser()
{
    struct passwd *pwd;
    pwd = getpwuid(getuid());
    username = QString(pwd->pw_name);
    ui->lblUserName->setText(username);

    ui->lblAvatar->setPixmap(QPixmap(":/images/assets/avatar.png"));
}

void MainWindow::initialize()
{
	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
    serviceInterface = new QDBusInterface(DBUS_SERVICE, DBUS_PATH,
                                          DBUS_INTERFACE,
                                          QDBusConnection::systemBus());
    serviceInterface->setTimeout(2147483647); /* 微秒 */

    initSysMenu();

    /* 获取并显示用户 */
    setCurrentUser();

	/* 获取设备列表 */
	getDeviceInfo();

	/* Other initializations */
	initDashboardBioAuthSection();
	initBiometricPage();
    initDeviceTypeList();

    connect(ui->btnMin, &QPushButton::clicked, this, &MainWindow::showMinimized);
    connect(ui->btnClose, &QPushButton::clicked, this, &MainWindow::close);

    ui->btnDashBoard->click();

    connect(serviceInterface, SIGNAL(USBDeviceHotPlug(int, int, int)),
            this, SLOT(onUSBDeviceHotPlug(int,int,int)));
}

void MainWindow::initSysMenu()
{
    menu = new QMenu(this);
    QAction *serviceStatusAction = new QAction(QIcon(":/images/assets/restart_service.png"),
                                               tr("Restart Service"), this);
    connect(serviceStatusAction, &QAction::triggered, this, [&]{
        if(restartService())
            updateDevice();
    });

    QAction *aboutAction = new QAction(QIcon(":images/assets/about.png"),
                                       tr("About"), this);
    connect(aboutAction, &QAction::triggered, this, [&]{
        if(aboutDlg == nullptr)
            aboutDlg = new AboutDialog();

        int x = this->geometry().topLeft().x() + (width() - aboutDlg->width()) / 2;
        int y = this->geometry().topLeft().y() + (height() - aboutDlg->height()) / 2;

        aboutDlg->move(x, y);
        aboutDlg->show();
        aboutDlg->raise();
    });

    menu->addActions({serviceStatusAction, aboutAction});
    ui->btnMenu->setMenu(menu);
}

void MainWindow::changeBtnColor(QPushButton *btn)
{
    if(btn == ui->btnDashBoard) {
        ui->btnDashBoard->setStyleSheet("background-color: #0066b8;");
        ui->btnDashBoard->setIcon(QIcon(":/images/assets/dashboard_click.png"));
    }
    else {
        ui->btnDashBoard->setStyleSheet("background-color: #0078d7;");
        ui->btnDashBoard->setIcon(QIcon(":/images/assets/dashboard_default.png"));
    }
    if(btn == ui->btnFingerPrint) {
        ui->btnFingerPrint->setStyleSheet("background-color: #0066b8;");
        ui->btnFingerPrint->setIcon(QIcon(":/images/assets/fingerprint_click.png"));
    }
    else {
        ui->btnFingerPrint->setStyleSheet("background-color: #0078d7;");
        ui->btnFingerPrint->setIcon(QIcon(":/images/assets/fingerprint_default.png"));
    }
    if(btn == ui->btnFingerVein) {
        ui->btnFingerVein->setStyleSheet("background-color: #0066b8;");
        ui->btnFingerVein->setIcon(QIcon(":/images/assets/fingervein_click.png"));
    }
    else {
        ui->btnFingerVein->setStyleSheet("background-color: #0078d7;");
        ui->btnFingerVein->setIcon(QIcon(":/images/assets/fingervein_default.png"));
    }
    if(btn == ui->btnIris) {
        ui->btnIris->setStyleSheet("background-color: #0066b8;");
        ui->btnIris->setIcon(QIcon(":/images/assets/iris_click.png"));
    }
    else {
        ui->btnIris->setStyleSheet("background-color: #0078d7;");
        ui->btnIris->setIcon(QIcon(":/images/assets/iris_default.png"));
    }
    if(btn == ui->btnVoicePrint) {
        ui->btnVoicePrint->setStyleSheet("background-color: #0066b8;");
        ui->btnVoicePrint->setIcon(QIcon(":/images/assets/voiceprint_click.png"));
    }
    else {
        ui->btnVoicePrint->setStyleSheet("background-color: #0078d7;");
        ui->btnVoicePrint->setIcon(QIcon(":/images/assets/voiceprint_default.png"));
    }
}

void MainWindow::on_btnDashBoard_clicked()
{
    ui->stackedWidgetMain->setCurrentWidget(ui->pageDashBoard);

    changeBtnColor(ui->btnDashBoard);
}

void MainWindow::on_btnFingerPrint_clicked()
{
    ui->stackedWidgetMain->setCurrentWidget(ui->pageFingerPrint);

    changeBtnColor(ui->btnFingerPrint);
}

void MainWindow::on_btnFingerVein_clicked()
{
    ui->stackedWidgetMain->setCurrentWidget(ui->pageFingerVein);

    changeBtnColor(ui->btnFingerVein);
}

void MainWindow::on_btnIris_clicked()
{
    ui->stackedWidgetMain->setCurrentWidget(ui->pageIris);

    changeBtnColor(ui->btnIris);
}

void MainWindow::on_btnVoicePrint_clicked()
{
    ui->stackedWidgetMain->setCurrentWidget(ui->pageVoicePrint);

    changeBtnColor(ui->btnVoicePrint);
}

/**
 * @brief 设备类型到索引的映射
 */
int MainWindow::bioTypeToIndex(int type)
{
    switch(type) {
    case BIOTYPE_FINGERPRINT:
        return 0;
    case BIOTYPE_FINGERVEIN:
        return 1;
    case BIOTYPE_IRIS:
        return 2;
    case BIOTYPE_VOICEPRINT:
        return 3;
    }
    return -1;
}

/**
 * @brief 获取设备列表并存储起来备用
 */
void MainWindow::getDeviceInfo()
{
	QVariant variant;
	QDBusArgument argument;
	QList<QDBusVariant> qlist;
	QDBusVariant item;
	DeviceInfo *deviceInfo;

	/* 返回值为 i -- int 和 av -- array of variant */
    QDBusPendingReply<int, QList<QDBusVariant> > reply = serviceInterface->call("GetDrvList");
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		deviceCount = 0;
		return;
	}

	/* 解析 DBus 返回值，reply 有两个返回值，都是 QVariant 类型 */
	variant = reply.argumentAt(0); /* 得到第一个返回值 */
	deviceCount = variant.value<int>(); /* 解封装得到设备个数 */
	variant = reply.argumentAt(1); /* 得到第二个返回值 */
	argument = variant.value<QDBusArgument>(); /* 解封装，获取QDBusArgument对象 */
	argument >> qlist; /* 使用运算符重载提取 argument 对象里面存储的列表对象 */

    for(int i = 0; i < __MAX_NR_BIOTYPES; i++)
        deviceInfosMap[i].clear();;

	for (int i = 0; i < deviceCount; i++) {
		item = qlist[i]; /* 取出一个元素 */
		variant = item.variant(); /* 转为普通QVariant对象 */
		/* 解封装得到 QDBusArgument 对象 */
		argument = variant.value<QDBusArgument>();
		deviceInfo = new DeviceInfo();
		argument >> *deviceInfo; /* 提取最终的 DeviceInfo 结构体 */
        deviceInfosMap[bioTypeToIndex(deviceInfo->biotype)].append(deviceInfo);

//        qDebug() << deviceInfo->biotype << deviceInfo->device_shortname << deviceInfo->device_available;
	}
}

void MainWindow::addContentPane(DeviceInfo *deviceInfo)
{
	QListWidget *lw;
	QStackedWidget *sw;

    switch(deviceInfo->biotype) {
    case BIOTYPE_FINGERPRINT:
        lw = ui->listWidgetFingerPrint;
        sw = ui->stackedWidgetFingerPrint;
        break;
    case BIOTYPE_FINGERVEIN:
        lw = ui->listWidgetFingerVein;
        sw = ui->stackedWidgetFingerVein;
        break;
    case BIOTYPE_IRIS:
        lw = ui->listWidgetIris;
        sw = ui->stackedWidgetIris;
        break;
    case BIOTYPE_VOICEPRINT:
        lw = ui->listWidgetVoicePrint;
        sw = ui->stackedWidgetVoicePrint;
        break;
    }

    QListWidgetItem *item = new QListWidgetItem(deviceInfo->device_shortname);
	item->setTextAlignment(Qt::AlignCenter);
	lw->insertItem(lw->count(), item);
    if(deviceInfo->device_available <= 0)
        item->setTextColor(Qt::gray);

    ContentPane *contentPane = new ContentPane(getuid(), deviceInfo);
	sw->addWidget(contentPane);
    contentPaneMap.insert(deviceInfo->device_shortname, contentPane);

    connect(lw, &QListWidget::currentRowChanged, sw, &QStackedWidget::setCurrentIndex);
    connect(contentPane, &ContentPane::changeDeviceStatus, this, &MainWindow::changeDeviceStatus);
}



#define checkBiometricPage(biometric) do {				\
	if (ui->listWidget##biometric->count() >= 1) {			\
		ui->listWidget##biometric->setCurrentRow(0);		\
		ui->listWidget##biometric->show();			\
		ui->stackedWidget##biometric->show();			\
		ui->lblNoDevice##biometric->hide();			\
	} else {							\
		ui->listWidget##biometric->hide();			\
		ui->stackedWidget##biometric->hide();			\
		ui->lblNoDevice##biometric->show();			\
	}								\
} while(0)

void MainWindow::initBiometricPage()
{
    for(int i = 0; i < __MAX_NR_BIOTYPES; i++)
        for (auto deviceInfo : deviceInfosMap[i])
            addContentPane(deviceInfo);
    checkBiometricPage(FingerPrint);
    checkBiometricPage(FingerVein);
	checkBiometricPage(Iris);
    checkBiometricPage(VoicePrint);
}

#define SET_TABLE_ATTRIBUTE(tw) do {					\
	tw->setColumnCount(2);						\
	tw->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);\
	tw->setHorizontalHeaderLabels(QStringList()			\
		<< QString(tr("Device Name")) << QString(tr("Status")));\
	tw->verticalHeader()->setVisible(false);			\
	tw->horizontalHeader()->setSectionResizeMode(1,			\
					QHeaderView::ResizeToContents);	\
	tw->setSelectionMode(QAbstractItemView::NoSelection);		\
} while (0);



void MainWindow::initDashboardBioAuthSection()
{
	QProcess process;
	process.start("bioctl status");
	process.waitForFinished();
	QString output = process.readAllStandardOutput();
    qDebug() << "bioctl status ---" << output;
    if (output.contains("enable", Qt::CaseInsensitive)) {
        setVerificationStatus(true);
    }
    else {
        setVerificationStatus(false);
    }
}

void MainWindow::initDeviceTypeList()
{
    QStringList devicesTypeText = {tr("FingerPrint"), tr("FingerVein"),
                                   tr("Iris"), tr("VoicePrint")};
    for(int i = 0; i < devicesTypeText.size(); i++)
        ui->listWidgetDevicesType->insertItem(ui->listWidgetDevicesType->count(),
                                              "    " + devicesTypeText[i]);

    ui->listWidgetDevicesType->setCurrentRow(0);
}

void MainWindow::setVerificationStatus(bool status)
{
    QString noteText, statusText, statusStyle;

    verificationStatus = status;

    if (status) {
        statusText = tr("Opened");
        noteText = tr("Biometric Authentication can take over system authentication processes "
                      "which include Login, LockScreen, sudo/su and Polkit");
        statusStyle = "background:url(:/images/assets/switch_open_large.png)";
    }
    else {
        statusText = tr("Closed");
        noteText = tr("There is no any available biometric device or no features enrolled currently.");
        statusStyle = "background:url(:/images/assets/switch_close_large.png)";
    }
    ui->lblNote->setText(noteText);
    ui->lblStatus->setText(statusText);
    ui->btnStatus->setStyleSheet(statusStyle);
}

void MainWindow::on_btnStatus_clicked()
{
    if(!verificationStatus){
        int featuresCount = 0;
        for(auto contentPane :contentPaneMap){
            featuresCount += contentPane->featuresCount();
        }
        qDebug() << "FeatureCount: " << featuresCount;
        if(featuresCount <= 0){
            MessageDialog msgDialog(MessageDialog::Error,
                            tr("Warnning"),
                            tr("There is no available device or no features enrolled"));
            msgDialog.exec();
            return;
        }
    }
    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (verificationStatus) {
        process.start("pkexec bioctl disable -u " + environment.value("USER"));
        process.waitForFinished(-1);
        if (process.exitCode() == 0)
            setVerificationStatus(false);
    } else {
        process.start("pkexec bioctl enable -u " + environment.value("USER"));
        process.waitForFinished(-1);
        if (process.exitCode() == 0)
            setVerificationStatus(true);
    }
}

void MainWindow::on_listWidgetDevicesType_currentRowChanged(int currentRow)
{
    int deviceType = (currentRow);
    QStringList headerData;
    headerData << "    " + tr("Device Name") << tr("Device Status") << tr("Driver Status") << tr("Default")
               << "    " + tr("Device Name") << tr("Device Status") << tr("Driver Status") << tr("Default");

    ui->tableWidgetDevices->clear();
    ui->tableWidgetDevices->setRowCount(0);
    ui->tableWidgetDevices->setColumnCount(8);
    ui->tableWidgetDevices->setHorizontalHeaderLabels(headerData);
    ui->tableWidgetDevices->setFocusPolicy(Qt::NoFocus);
    for(int i = 0; i < headerData.size(); i++){
        if(i % 4 == 0)
            ui->tableWidgetDevices->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        else
            ui->tableWidgetDevices->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    }
    ui->tableWidgetDevices->setColumnWidth(0, 100);
    ui->tableWidgetDevices->setColumnWidth(1, 100);
    ui->tableWidgetDevices->setColumnWidth(2, 100);
    ui->tableWidgetDevices->setColumnWidth(3, 80);
    ui->tableWidgetDevices->setColumnWidth(4, 100);
    ui->tableWidgetDevices->setColumnWidth(5, 100);
    ui->tableWidgetDevices->setColumnWidth(6, 100);
    ui->tableWidgetDevices->setColumnWidth(7, 50);
    int column = 0;
    for(auto deviceInfo : deviceInfosMap[deviceType]) {
        if(bioTypeToIndex(deviceInfo->biotype) == deviceType) {
            int row_index = ui->tableWidgetDevices->rowCount();
            if(column == 0)
                ui->tableWidgetDevices->insertRow(row_index);
            else
                row_index--;

            //第一、五列 设备名称
            QTableWidgetItem *item_name = new QTableWidgetItem("   " + deviceInfo->device_shortname);
            item_name->setFlags(item_name->flags() ^ Qt::ItemIsEditable);
            ui->tableWidgetDevices->setItem(row_index, column, item_name);

            //第二、六列 设备状态（是否连接）
            QTableWidgetItem *item_devStatus = new QTableWidgetItem;
            setDeviceStatus(item_devStatus, deviceInfo->device_available > 0);
            item_devStatus->setFlags(item_name->flags() ^ Qt::ItemIsEditable);
            item_devStatus->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            ui->tableWidgetDevices->setItem(row_index, column + 1, item_devStatus);

            //第三、七列 驱动状态（是否使能）
            QWidget *item_drvStatus = new QWidget();
            QPushButton *btnDrvStatus = new QPushButton(this);
            btnDrvStatus->setObjectName(deviceInfo->device_shortname + "_" + QString::number(deviceType));
            btnDrvStatus->setFixedSize(40, 20);
            if(deviceInfo->driver_enable > 0)
                btnDrvStatus->setStyleSheet("background:url(:/images/assets/switch_open_small.png)");
            else
                btnDrvStatus->setStyleSheet("background:url(:/images/assets/switch_close_small.png)");
            connect(btnDrvStatus, &QPushButton::clicked, this, &MainWindow::onDriverStatusClicked);

            QVBoxLayout *layout = new QVBoxLayout(item_drvStatus);
            layout->addWidget(btnDrvStatus, 0, Qt::AlignVCenter | Qt::AlignHCenter);
            layout->setMargin(0);
            item_drvStatus->setLayout(layout);
            ui->tableWidgetDevices->setCellWidget(row_index, column + 2, item_drvStatus);

            //第四、八列 默认设备（是否设为默认设备）
            QWidget *item_default = new QWidget(ui->tableWidgetDevices);
            item_default->setObjectName("itemDefalut");
            if(column == 0)
                item_default->setStyleSheet("#itemDefalut{border-right: 1px solid lightgray; }");
            QCheckBox *cbDefault = new QCheckBox(this);

            if(Configuration::instance()->getDefaultDevice() == deviceInfo->device_shortname)
                cbDefault->setChecked(true);
            btnGroup.push_back(cbDefault);
            cbDefault->setObjectName("cb_" + deviceInfo->device_shortname);
            connect(cbDefault, &QCheckBox::clicked, this, &MainWindow::onDefaultDeviceChanged);
            connect(Configuration::instance(), &Configuration::defaultDeviceChanged,
                    this, [&](const QString &deviceName) {
                QString objName = "cb_" + deviceName;
                QCheckBox *check = findChild<QCheckBox*>(objName);
                if(check) {
                    check->setChecked(true);
                }
                else {
                    for(auto btn : btnGroup)
                        btn->setChecked(false);
                }
            });

            QHBoxLayout *layout_default = new QHBoxLayout;
            layout_default->addWidget(cbDefault);
            layout_default->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
            item_default->setLayout(layout_default);
            ui->tableWidgetDevices->setCellWidget(row_index, column + 3, item_default);

            column = (column + 4) % 8;
        }
    }
}

void MainWindow::onDriverStatusClicked()
{
    QString objNameStr = sender()->objectName();
    qDebug() << objNameStr;
    int spliter = objNameStr.lastIndexOf('_');
    QString deviceName = objNameStr.left(spliter);
    int deviceType = objNameStr.right(objNameStr.length() - spliter - 1).toInt();

    if(deviceType != ui->listWidgetDevicesType->currentRow())
        return;
    DeviceInfo *deviceInfo;
    for(auto info : deviceInfosMap[deviceType]) {
        if(info->device_shortname == deviceName) {
            deviceInfo = info;
            break;
        }
    }

    changeDeviceStatus(deviceInfo);
}

void MainWindow::onDefaultDeviceChanged(bool checked)
{
    QString objName = sender()->objectName();
    QString deviceName = objName.split("_").at(1);

    for(auto cb : btnGroup)
        if(cb != sender())
            cb->setChecked(false);

    if(checked)
        Configuration::instance()->setDefaultDevice(deviceName);
    else
        Configuration::instance()->setDefaultDevice("");
}


bool MainWindow::changeDeviceStatus(DeviceInfo *deviceInfo)
{
    bool toEnable = deviceInfo->driver_enable <= 0 ? true : false;
    QProcess process;
    QString cmd;
    if (toEnable) {
        qDebug() << "enable" << deviceInfo->device_shortname;
        cmd = "pkexec biodrvctl enable " //"pkexec biometric-config-tool enable-driver "
                + deviceInfo->device_shortname;
        qDebug() << cmd;
        process.start(cmd);
        process.waitForFinished(-1);
    } else {
        qDebug() << "disable" << deviceInfo->device_shortname;
        cmd = "pkexec biodrvctl disable " //"pkexec biometric-config-tool disable-driver "
                + deviceInfo->device_shortname;
        qDebug() << cmd;
        process.start(cmd);
        process.waitForFinished(-1);
    }
    if (process.exitCode() != 0) {
        MessageDialog msgDialog(MessageDialog::Error,
                            tr("Fatal Error"),
                            tr("Fail to change device status"));
        msgDialog.exec();
        return false;
    }
//    MessageDialog msgDialog(MessageDialog::Question,
//                            tr("Restart Service"),
//                            tr("The configuration has been modified. "
//                               "Restart the service immediately to make it effecitve?"));
//    msgDialog.setOkText(tr("  Restart immediately  "));
//    msgDialog.setCancelText(tr("  Restart later  "));
//    int status = msgDialog.exec();
//    if(status == MessageDialog::Rejected) {
//        return false;
//    } else {
//        if(!restartService())
//            return false;
//    }

    updateDevice();

    /*
     * There is a condition that the driver is enabled while the device is
     * not connected. So, if user enables the driver, we need to update the
     * deviceinfo array to make sure the device is indeed available. If user
     * disabled the driver, the device must can't be used and therefor we
     * don't need to query device info from DBus.
     */
//    if(toEnable) {
//updateStatus:
//        QDBusMessage reply = serviceInterface->call("UpdateStatus", deviceInfo->device_id);
//        if(reply.type() == QDBusMessage::ErrorMessage)
//            qDebug() << "UpdateStatus error: " << reply.errorMessage();

//        //等待服务重启后DBus启动
//        if(reply.arguments().length() < 3) {
//            usleep(200000);
//            goto updateStatus;
//        }

//        int result = reply.arguments().at(0).toInt();
//        deviceInfo->device_available = reply.arguments().at(2).toInt();

//        if(result == DBUS_RESULT_NOSUCHDEVICE){
//            MessageDialog msgDialog(MessageDialog::Error,
//                                    tr("Error"),
//                                    tr("Device is not connected"));
//            msgDialog.exec();
//            return false;
//        }
//    } else {
//        deviceInfo->device_available = 0;
//    }

    return true;
}

bool MainWindow::restartService()
{
//    QDBusInterface interface("org.freedesktop.systemd1",
//                             "/org/freedesktop/systemd1",
//                             "org.freedesktop.systemd1.Manager",
//                             QDBusConnection::systemBus());
//    QDBusReply<QDBusObjectPath> msg = interface.call("RestartUnit", "biometric-authentication.service", "replace");
//    if(!msg.isValid()) {
//        qDebug() << "restart service: " << msg.error();
//        return false;
//    }
    QProcess process;
    QString cmd = QString("pkexec systemctl restart %1").arg(SERVICE);
    process.start(cmd);
    process.waitForFinished(-1);
    qDebug() << "restart service finished";
    return true;
}

void MainWindow::updateDeviceListWidget(int biotype)
{
    QListWidget *lw = nullptr;

    switch(biotype) {
    case BIOTYPE_FINGERPRINT:
        lw = ui->listWidgetFingerPrint;
        break;
    case BIOTYPE_FINGERVEIN:
        lw = ui->listWidgetFingerVein;
        break;
    case BIOTYPE_IRIS:
        lw = ui->listWidgetIris;
        break;
    case BIOTYPE_VOICEPRINT:
        lw = ui->listWidgetVoicePrint;
        break;
    }
    if(!lw) return;

    for(int row = 0; row < lw->count(); row++)
    {
        QListWidgetItem *item = lw->item(row);
        QList<DeviceInfo *> list = deviceInfosMap[bioTypeToIndex(biotype)];
        auto iter = std::find_if(list.begin(), list.end(),
                              [&](DeviceInfo *deviceInfo){
                return deviceInfo->device_shortname == item->text(); });

        item->setTextColor((*iter)->device_available ? Qt::black : Qt::gray);
    }
}

void MainWindow::updateDevice()
{
    setCursor(Qt::WaitCursor);
    sleep(3);   //wait for service restart and dbus is ready
    getDeviceInfo();
    on_listWidgetDevicesType_currentRowChanged(ui->listWidgetDevicesType->currentRow());
    for(int i = 0; i < __MAX_NR_BIOTYPES; i++){
        for(auto deviceInfo : deviceInfosMap[i]){
            ContentPane *contentPane = contentPaneMap[deviceInfo->device_shortname];
            contentPane->setDeviceInfo(deviceInfo);
            contentPane->showFeatures();
        }
        updateDeviceListWidget(i);
    }
    setCursor(Qt::ArrowCursor);
}

void MainWindow::on_tableWidgetDevices_cellDoubleClicked(int row, int column)
{
    if(column % 4 != 0)
        return;
    int index = row * 2 + column / 4;

    if(index < deviceInfosMap[ui->listWidgetDevicesType->currentRow()].size()) {
        int deviceType = ui->listWidgetDevicesType->currentRow();
        DeviceInfo *deviceInfo =  deviceInfosMap[deviceType][index];
        QListWidget *lw;
        switch(deviceInfo->biotype) {
        case BIOTYPE_FINGERPRINT:
            lw = ui->listWidgetFingerPrint;
            ui->btnFingerPrint->click();
            break;
        case BIOTYPE_FINGERVEIN:
            lw = ui->listWidgetFingerVein;
            ui->btnFingerVein->click();
            break;
        case BIOTYPE_IRIS:
            lw = ui->listWidgetIris;
            ui->btnIris->click();
            break;
        case BIOTYPE_VOICEPRINT:
            lw = ui->listWidgetVoicePrint;
            ui->btnVoicePrint->click();
            break;
        }
        lw->setCurrentRow(index);
    }
}

void MainWindow::onUSBDeviceHotPlug(int drvid, int action, int devNumNow)
{
    qDebug() << "device"<< (action > 0 ? "insert:" : "pull out:");
    qDebug() << "id:" << drvid;
    for(int type : deviceInfosMap.keys()) {
        auto &deviceInfoList = deviceInfosMap[type];
        for(int i = 0; i < deviceInfoList.size(); i++) {
            auto deviceInfo = deviceInfoList[i];
            if(deviceInfo->device_id == drvid) {
                qDebug() << "name:" << deviceInfo->device_shortname;

                //更新结构体
                deviceInfo->device_available = devNumNow;
                //更新标签页中的设备状态
                ContentPane *pane = contentPaneMap[deviceInfo->device_shortname];
                pane->setDeviceAvailable(devNumNow);

                if(type != ui->listWidgetDevicesType->currentRow())
                {
                    return;
                }

                int row = i / 2;
                int column = i % 2 == 0 ? 1 : 5;
                //更新表中的设备状态
                QTableWidgetItem *item = ui->tableWidgetDevices->item(row, column);
                setDeviceStatus(item, devNumNow > 0);
                return;
            }
        }
    }
}

void MainWindow::setDeviceStatus(QTableWidgetItem *item, bool connected)
{
    QString deviceStatus;
    if(connected) {
        deviceStatus = tr("Connected");
        item->setTextColor(Qt::red);
    }
    else {
        deviceStatus = tr("Unconnected");
        item->setTextColor(Qt::black);
    }
    item->setText(deviceStatus);
}


void MainWindow::onServiceStatusChanged(bool activate)
{
    if(!activate)
    {
        ui->stackedWidgetMain->hide();
        lblPrompt = new QLabel(this);
        lblPrompt->setGeometry(ui->stackedWidgetMain->x(),ui->stackedWidgetMain->y(),
                               ui->stackedWidgetMain->width(),
                               ui->stackedWidgetMain->height());
        lblPrompt->setText(tr("The Service is stopped"));
        lblPrompt->setAlignment(Qt::AlignCenter);
        lblPrompt->setStyleSheet("QLabel{color: red; font-size: 20px;}");
        lblPrompt->show();
    }
    else
    {
        lblPrompt->hide();
        ui->stackedWidgetMain->show();
    }
}
