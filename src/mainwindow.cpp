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
#include <QDBusInterface>
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
    lastDeviceInfo(nullptr),
    aboutDlg(nullptr)
{
	ui->setupUi(this);
	prettify();

    initialize();
    setWindowIcon(QIcon::fromTheme("biometric-manager"));
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

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_F1) {
        if(!daemonIsNotRunning()){
            showGuide("biometric-manager");
        }
    }
}

int MainWindow::daemonIsNotRunning()
{
    QString service_name = "com.kylinUserGuide.hotel_" + QString::number(getuid());
    QDBusConnection conn = QDBusConnection::sessionBus();
    if (!conn.isConnected())
        return 0;

    QDBusReply<QString> reply = conn.interface()->call("GetNameOwner", service_name);
    return reply.value() == "";
}


void MainWindow::showGuide(QString appName)
{
    qDebug() << Q_FUNC_INFO << appName;

    QString service_name = "com.kylinUserGuide.hotel_" + QString::number(getuid());

    QDBusInterface *interface = new QDBusInterface(service_name,
                                                       KYLIN_USER_GUIDE_PATH,
                                                       KYLIN_USER_GUIDE_INTERFACE,
                                                       QDBusConnection::sessionBus(),
                                                       this);

    QDBusMessage msg = interface->call(QStringLiteral("showGuide"),"biometric-manager");
}

void MainWindow::mouseReleaseEvent(QMouseEvent */*event*/)
{
    dragWindow = false;
}

void MainWindow::prettify()
{
    setWindowFlags(Qt::WindowCloseButtonHint);
	/* 设置窗口图标 */
    QApplication::setWindowIcon(QIcon::fromTheme("biometric-manager"));
	/* 设置 CSS */
	QFile qssFile(":/css/assets/mainwindow.qss");
	qssFile.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(qssFile.readAll());
	this->setStyleSheet(styleSheet);
	qssFile.close();

    ui->lblTitle->setText(tr("Biometric Manager"));
	/* Set Icon for each tab on tabwidget */
    ui->btnDashBoard->setIcon(QIcon(":/images/assets/dashboard.png"));
    ui->btnFingerPrint->setIcon(QIcon(":/images/assets/fingerprint.png"));
    ui->btnFingerVein->setIcon(QIcon(":/images/assets/fingervein.png"));
    ui->btnIris->setIcon(QIcon(":/images/assets/iris.png"));
    ui->btnVoicePrint->setIcon(QIcon(":/images/assets/voiceprint.png"));
//    /* Set logo on lblLogo */
    ui->lblLogo->setPixmap(QIcon::fromTheme("biometric-manager").pixmap(QSize(24,24)));
 //   ui->btnMin->setIcon(QIcon(":/images/assets/min.png"));
    ui->btnMin->setProperty("isWindowButton", 0x1);
    ui->btnMin->setProperty("useIconHighlightEffect", 0x2);
    ui->btnMin->setProperty("setIconHighlightEffectDefaultColor", ui->btnMin->palette().color(QPalette::Active, QPalette::Base));
    ui->btnMin->setFixedSize(30, 30);
    ui->btnMin->setAutoRaise(true);
    ui->btnMin->setIconSize(QSize(16, 16));
    ui->btnMin->setIcon(QIcon::fromTheme("window-minimize-symbolic"));

  //  ui->btnClose->setIcon(QIcon(":/images/assets/close.png"));
    ui->btnClose->setProperty("isWindowButton", 0x2);
    ui->btnClose->setProperty("useIconHighlightEffect", 0x8);
    ui->btnClose->setProperty("setIconHighlightEffectDefaultColor", ui->btnClose->palette().color(QPalette::Active, QPalette::Base));
    ui->btnClose->setFixedSize(30, 30);
    ui->btnClose->setAutoRaise(true);
   // ui->btnClose->setIconSize(QSize(16, 16));
    ui->btnClose->setIcon(QIcon::fromTheme("window-close-symbolic"));

  //  ui->btnMenu->setIcon(QIcon(":/images/assets/menu.png"));
    ui->btnMenu->setProperty("isWindowButton", 0x1);
    ui->btnMenu->setProperty("useIconHighlightEffect", 0x2);
    ui->btnMenu->setProperty("setIconHighlightEffectDefaultColor", ui->btnMenu->palette().color(QPalette::Active, QPalette::Base));
    ui->btnMenu->setFixedSize(30, 30);
    ui->btnMenu->setIconSize(QSize(16, 16));
    ui->btnMenu->setAutoRaise(true);
    ui->btnMenu->setIcon(QIcon::fromTheme("open-menu-symbolic"));
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
//    struct passwd *pwd;
//    pwd = getpwuid(getuid());
//    username = QString(pwd->pw_name);
//    ui->lblUserName->setText(username);

//    ui->lblAvatar->setPixmap(QPixmap(":/images/assets/avatar.png"));
}

void MainWindow::onReviceWindowMessage(QString message)
{
    if (!this->isActiveWindow())
    {
        this->hide();
        this->show();
        activateWindow();
    }

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
    QAction *serviceStatusAction = new QAction(tr("Restart Service"), this);
    connect(serviceStatusAction, &QAction::triggered, this, [&]{
        if(restartService())
            updateDevice();
    });

    QAction *aboutAction = new QAction(tr("About"), this);
    connect(aboutAction, &QAction::triggered, this, [&]{
        if(aboutDlg == nullptr)
            aboutDlg = new AboutDialog();

        int x = this->geometry().topLeft().x() + (width() - aboutDlg->width()) / 2;
        int y = this->geometry().topLeft().y() + (height() - aboutDlg->height()) / 2;

        aboutDlg->move(x, y);
        aboutDlg->exec();
    });

    QAction *exitAction = new QAction(tr("exit"), this);
    connect(exitAction, &QAction::triggered, this, [&]{
        close();
    });

    QAction *helpAction = new QAction(tr("help"), this);
    connect(helpAction, &QAction::triggered, this, [&]{
        if(!daemonIsNotRunning()){
            showGuide("biometric-manager");
        }
    });

    menu->addActions({serviceStatusAction, helpAction,aboutAction,exitAction});
    ui->btnMenu->setPopupMode(QToolButton::InstantPopup   );
    ui->btnMenu->setMenu(menu);
}

void MainWindow::changeBtnColor(QPushButton *btn)
{
    if(btn != ui->btnDashBoard) {
        ui->btnDashBoard->setStyleSheet("background-color: #3d6be5;color:#ffffff");
        ui->btnDashBoard->setIcon(QIcon(":/images/assets/dashboard-white.png"));
    }
    else {
        ui->btnDashBoard->setIcon(QIcon(":/images/assets/dashboard.png"));
        ui->btnDashBoard->setStyleSheet("QPushButton{border:none;background-color:#ffffff;}"
                                        "QPushButton:hover{background-color:#ffffff;border:none;}");
    }
    if(btn != ui->btnFingerPrint) {
        ui->btnFingerPrint->setStyleSheet("background-color: #3d6be5;color:#ffffff");
        ui->btnFingerPrint->setIcon(QIcon(":/images/assets/fingerprint-white.png"));
    }
    else {
        ui->btnFingerPrint->setIcon(QIcon(":/images/assets/fingerprint.png"));
        ui->btnFingerPrint->setStyleSheet("QPushButton{border:none;background-color:#ffffff;}"
                                          "QPushButton:hover{background-color:#ffffff;border:none;}");
    }
    if(btn != ui->btnFingerVein) {
        ui->btnFingerVein->setIcon(QIcon(":/images/assets/fingervein-white.png"));
        ui->btnFingerVein->setStyleSheet("background-color: #3d6be5;color:#ffffff");
    }
    else {
        ui->btnFingerVein->setIcon(QIcon(":/images/assets/fingervein.png"));
        ui->btnFingerVein->setStyleSheet("QPushButton{border:none;background-color:#ffffff;}"
                                         "QPushButton:hover{background-color:#ffffff;border:none;}");
    }
    if(btn != ui->btnIris) {
        ui->btnIris->setIcon(QIcon(":/images/assets/iris-white.png"));
        ui->btnIris->setStyleSheet("background-color: #3d6be5;color:#ffffff");
    }
    else {
        ui->btnIris->setIcon(QIcon(":/images/assets/iris.png"));
        ui->btnIris->setStyleSheet("QPushButton{border:none;background-color:#ffffff;}"
                                   "QPushButton:hover{background-color:#ffffff;border:none;}");
    }
    if(btn != ui->btnVoicePrint) {
        ui->btnVoicePrint->setIcon(QIcon(":/images/assets/voiceprint-white.png"));
        ui->btnVoicePrint->setStyleSheet("background-color: #3d6be5;color:#ffffff");
    }
    else {
        ui->btnVoicePrint->setIcon(QIcon(":/images/assets/voiceprint.png"));
        ui->btnVoicePrint->setStyleSheet("QPushButton{border:none;background-color:#ffffff;}"
                                         "QPushButton:hover{background-color:#ffffff;border:none;}");
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

void MainWindow::setLastDeviceSelected()
{
    if(!lastDeviceInfo)
        return ;

    QListWidget *lw;
    QStackedWidget *sw;

    switch(lastDeviceInfo->biotype) {
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

    for(int i=0;i<lw->count();i++)
    {
        if(lw->item(i)->text()==lastDeviceInfo->device_shortname)
        {
            lw->setCurrentRow(i);
            sw->setCurrentIndex(i);
        }
    }

    lastDeviceInfo = nullptr;
}

void MainWindow::raiseContentPane(DeviceInfo *deviceInfo)
{
    if(deviceInfo->device_available<=0)
        return ;

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

    for(int i=0;i<lw->count();i++)
    {
        if(lw->item(i)->text()==deviceInfo->device_shortname)
        {
            QListWidgetItem *item = lw->takeItem(i);
            lw->insertItem(0,item);
            QWidget *widget = sw->widget(i);
            sw->removeWidget(sw->widget(i));
            sw->insertWidget(0,widget);
        }
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
    ContentPane *contentPane = new ContentPane(getuid(), deviceInfo);
	item->setTextAlignment(Qt::AlignCenter);
    if(deviceInfo->device_available <= 0){
        lw->insertItem(lw->count(), item);
        sw->insertWidget(sw->count(),contentPane);
    }
    else {
        lw->insertItem(0,item);
        sw->insertWidget(0,contentPane);
    }
    if(deviceInfo->device_available <= 0)
        item->setTextColor(Qt::gray);

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
    ui->listWidgetFingerPrint->clear();
    for(int i = ui->stackedWidgetFingerPrint->count(); i >= 0; i--)
    {
        QWidget* widget = ui->stackedWidgetFingerPrint->widget(i);
        ui->stackedWidgetFingerPrint->removeWidget(widget);
        widget->deleteLater();
    }
    ui->listWidgetFingerVein->clear();
    for(int i = ui->stackedWidgetFingerVein->count(); i >= 0; i--)
    {
        QWidget* widget = ui->stackedWidgetFingerVein->widget(i);
        ui->stackedWidgetFingerVein->removeWidget(widget);
        widget->deleteLater();
    }
    ui->listWidgetIris->clear();
    for(int i = ui->stackedWidgetIris->count(); i >= 0; i--)
    {
        QWidget* widget = ui->stackedWidgetIris->widget(i);
        ui->stackedWidgetIris->removeWidget(widget);
        widget->deleteLater();
    }
    ui->listWidgetVoicePrint->clear();
    for(int i = ui->stackedWidgetVoicePrint->count(); i >= 0; i--)
    {
        QWidget* widget = ui->stackedWidgetVoicePrint->widget(i);
        ui->stackedWidgetVoicePrint->removeWidget(widget);
        widget->deleteLater();
    }
    contentPaneMap.clear();

    for(int i = 0; i < __MAX_NR_BIOTYPES; i++){
        for (auto deviceInfo : deviceInfosMap[i]){
            ContentPane *contentPane = contentPaneMap[deviceInfo->device_shortname];
            addContentPane(deviceInfo);
        }
    }
    checkBiometricPage(FingerPrint);
    checkBiometricPage(FingerVein);
	checkBiometricPage(Iris);
    checkBiometricPage(VoicePrint);
}

void MainWindow::sortContentPane()
{
    for(int i = 0; i < __MAX_NR_BIOTYPES; i++)
        for (auto deviceInfo : deviceInfosMap[i])
            raiseContentPane(deviceInfo);
    checkBiometricPage(FingerPrint);
    checkBiometricPage(FingerVein);
    checkBiometricPage(Iris);
    checkBiometricPage(VoicePrint);

    if(lastDeviceInfo)
        setLastDeviceSelected();
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
       noteText = tr("Process of using biometrics 1.Confirm that the device is connected \
2.Set the connected device as the default 3. The biometric status is to be turned on. 4.Finally enter the fingerprint");
       statusStyle = "background:url(:/images/assets/switch_close_large.png)";
   }
   ui->lblNote->setText(noteText);
   ui->lblStatus->setText(statusText);
   ui->btnStatus->setStyleSheet(statusStyle);
}

void MainWindow::on_btnStatus_clicked()
{
    QProcess process;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (verificationStatus) {
        process.start("bioctl disable");
        process.waitForFinished(-1);
        if (process.exitCode() == 0)
            setVerificationStatus(false);
    } else {
        process.start("bioctl enable");
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
    ui->tableWidgetDevices->hide();
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
    
    while(!btnGroup.isEmpty())
    {
           QCheckBox *box = btnGroup.last();
           btnGroup.removeLast();
           delete  box;
    }
    
    for(int i=0;i<deviceInfosMap[deviceType].count();i++)
    {
        if(deviceInfosMap[deviceType].at(i)->device_available > 0)
        {
            deviceInfosMap[deviceType].move(i,0);
        }
    } 
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
                btnDrvStatus->setStyleSheet("background:url(:/images/assets/switch_open_small.png);text-align:center;border: none;outline: none;background-repeat:no-repeat;");
            else
                btnDrvStatus->setStyleSheet("background:url(:/images/assets/switch_close_small.png);text-align:center;border: none;outline: none;background-repeat:no-repeat;");
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
            cbDefault->setObjectName(deviceInfo->device_shortname);
            connect(cbDefault, &QCheckBox::clicked, this, &MainWindow::onDefaultDeviceChanged);
            connect(Configuration::instance(), &Configuration::defaultDeviceChanged,
                    this, [&](const QString &deviceName) {
                for(auto btn : btnGroup)
                    btn->setChecked(false);

                QString objName = deviceName;
                QCheckBox *check = findChild<QCheckBox*>(objName);
                if(check) {
                    check->setChecked(true);
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
    ui->tableWidgetDevices->show();
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
    QString deviceName = objName;

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
                            tr("Fail to change device status"),this);
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
    lastDeviceInfo = deviceInfo;
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
    QString cmd = QString("pkexec biorestart");
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
        if(iter != std::end(list))
            item->setTextColor((*iter)->device_available ? Qt::black : Qt::gray);

    }
}

void MainWindow::updateDevice()
{
    setCursor(Qt::WaitCursor);
    sleep(3);   //wait for service restart and dbus is ready
    getDeviceInfo();
    on_listWidgetDevicesType_currentRowChanged(ui->listWidgetDevicesType->currentRow());

   /* for(int i = 0; i < __MAX_NR_BIOTYPES; i++){
        for(auto deviceInfo : deviceInfosMap[i]){
                ContentPane *contentPane = contentPaneMap[deviceInfo->device_shortname];
                if(contentPane){
            		contentPane->setDeviceInfo(deviceInfo);
            		contentPane->showFeatures();
                }else{
                    addContentPane(deviceInfo);
                }
        }
        updateDeviceListWidget(i);
    }*/
    initBiometricPage();
    setCursor(Qt::ArrowCursor);
     sortContentPane();
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
                if(devNumNow > 0 ){
                    sortContentPane();
                    on_listWidgetDevicesType_currentRowChanged(ui->listWidgetDevicesType->currentRow());
                }
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
