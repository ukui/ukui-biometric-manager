#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDBusArgument>
#include "customtype.h"
#include <QFile>
#include <QProcessEnvironment>
#include "contentpane.h"
#include "toggleswitch.h"
#include <unistd.h>
#include <QScrollBar>
#include <QMessageBox>

#define ICON_SIZE 32
#define DEVICE_TS_W 45 /* The width of ToggleSwitch in device list */
#define DEVICE_TS_H 26

MainWindow::MainWindow(QString usernameFromCmd, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	usernameFromCmd(usernameFromCmd)
{
	ui->setupUi(this);
	prettify();

	QProcess process;
	process.start("systemctl is-active biometric-authentication.service");
	process.waitForFinished();
	QString output = process.readAllStandardOutput();
	bool systemdActive = output.startsWith("active");

	if (systemdActive)
		initialize();
	else
		disableBiometricTabs();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::checkAPICompatibility()
{
	QDBusPendingReply<int> reply = biometricInterface->CheckAppApiVersion(1, 3, 9);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}
	int result = reply.argumentAt(0).value<int>();
	if (result != 0) {
		QMessageBox *messageBox = new QMessageBox(QMessageBox::Critical,
							tr("Fatal Error"),
							tr("API version is not compatible"),
							QMessageBox::Ok);
		messageBox->exec();
		/* https://stackoverflow.com/a/31081379/4112667 */
		QTimer::singleShot(0, qApp, &QCoreApplication::quit);
	}
}

void MainWindow::prettify()
{
	/* 设置窗口图标 */
	QApplication::setWindowIcon(QIcon(":/images/assets/icon.png"));
	/* 设置 CSS */
	QFile qssFile(":/css/assets/mainwindow.qss");
	qssFile.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(qssFile.readAll());
	this->setStyleSheet(styleSheet);
	qssFile.close();

	/* Set Icon for each tab on tabwidget */
	ui->tabWidgetMain->setTabIcon(0, QIcon(":/images/assets/tab-dashboard.png"));
	ui->tabWidgetMain->setTabIcon(1, QIcon(":/images/assets/tab-fingerprint.png"));
	ui->tabWidgetMain->setTabIcon(2, QIcon(":/images/assets/tab-fingervein.png"));
	ui->tabWidgetMain->setTabIcon(3, QIcon(":/images/assets/tab-iris.png"));
	ui->tabWidgetMain->setIconSize(QSize(32, 32));
	/* Set icon on lblIcon */
	ui->lblIcon->setPixmap(QPixmap(":/images/assets/icon.png").scaled(QSize(64, 64)));
}

QIcon *MainWindow::getUserAvatar(QString username)
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
	return new QIcon(iconPath);
}

/**
 * @brief 获取并显示用户列表
 */
void MainWindow::showUserList()
{
	/* The style sheet won't be applied to the combobox item view without this line. */
	ui->comboBoxUsername->setView(new QListView());
	/* Set the size of QIcon which is in item */
	ui->comboBoxUsername->setIconSize(QSize(45, 45));
	/* Set the font of combobox */
	ui->comboBoxUsername->setFont(QFont("Monospace", 16, QFont::Bold));
	/* Set font size of item */
	ui->comboBoxUsername->view()->setFont(QFont("Monospace", 16, QFont::Bold));

	QFile file("/etc/passwd");
	QString line;
	QStringList fields;
	QString username;
	int uid;

	if(!file.open(QIODevice::ReadOnly)) {
		qDebug() << "GUI:" << "/etc/passwd can not be opened";
	}

	QTextStream in(&file);

	/* 阻止 addItem 触发 currentIndexChanged 信号 */
	ui->comboBoxUsername->blockSignals(true);
	while(!in.atEnd()) {
		line = in.readLine();
		fields = line.split(":");
		username = fields[0];
		uid = fields[2].toInt();
		if (uid == 65534) /* nobody 用户 */
			continue;
		if (uid >=1000 || uid == 0)
			ui->comboBoxUsername->addItem(*getUserAvatar(username),
							username, QVariant(uid));
	}
	file.close();
	ui->comboBoxUsername->blockSignals(false);
}

void MainWindow::setDefaultUser()
{
	/* 设置下拉列表的当前项，触发 currentIndexChanged 信号 */
	if (usernameFromCmd == "") {
		/* 获取当前执行程序的用户名 */
		QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
		ui->comboBoxUsername->setCurrentText(environment.value("USER"));
	} else {
		ui->comboBoxUsername->setCurrentText(usernameFromCmd);
	}
}

/**
 * @brief 切换 ComboBox 的 Slot
 * @param index
 */
void MainWindow::on_comboBoxUsername_currentIndexChanged(int index)
{
	qDebug() << "GUI:" << "Username ComboBox Changed";
	int uid = ui->comboBoxUsername->itemData(index).toInt();
	emit selectedUserChanged(uid);
}

void MainWindow::initialize()
{
	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
	biometricInterface = new cn::kylinos::Biometric("cn.kylinos.Biometric",
							"/cn/kylinos/Biometric",
							QDBusConnection::systemBus(),
							this);
	biometricInterface->setTimeout(2147483647); /* 微秒 */

	checkAPICompatibility();

	/* 获取设备列表 */
	getDeviceInfo();

	/* Other initializations */
	initDashboardBioAuthSection();
	initDashboardDeviceSection();
	initBiometricPage();

	/* 获取并显示用户列表 */
	showUserList();
	setDefaultUser();
}

void MainWindow::enableBiometricTabs()
{
	ui->tabWidgetMain->setTabEnabled(1, true);
	ui->tabWidgetMain->setTabEnabled(2, true);
	ui->tabWidgetMain->setTabEnabled(3, true);
}

void MainWindow::disableBiometricTabs()
{
	ui->tabWidgetMain->setTabEnabled(1, false);
	ui->tabWidgetMain->setTabEnabled(2, false);
	ui->tabWidgetMain->setTabEnabled(3, false);
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
	QDBusPendingReply<int, QList<QDBusVariant> > reply = biometricInterface->GetDrvList();
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

	deviceInfoMap.clear();
	for (int i = 0; i < deviceCount; i++) {
		item = qlist[i]; /* 取出一个元素 */
		variant = item.variant(); /* 转为普通QVariant对象 */
		/* 解封装得到 QDBusArgument 对象 */
		argument = variant.value<QDBusArgument>();
		deviceInfo = new DeviceInfo();
		argument >> *deviceInfo; /* 提取最终的 DeviceInfo 结构体 */
		if (deviceInfo->device_available != 1)
			deviceInfo->device_available = 0;
		deviceInfoMap.insert(deviceInfo->device_shortname, deviceInfo);
	}
}

void MainWindow::addContentPane(QString deviceName)
{
	QListWidget *lw;
	QStackedWidget *sw;
	DeviceInfo *deviceInfo = deviceInfoMap.value(deviceName);
	if (deviceInfo->biotype == BIOTYPE_FINGERPRINT) {
		lw = ui->listWidgetFingerprint;
		sw = ui->stackedWidgetFingerprint;
	} else if (deviceInfo->biotype == BIOTYPE_FINGERVEIN) {
		lw = ui->listWidgetFingervein;
		sw = ui->stackedWidgetFingervein;
	} else {
		lw = ui->listWidgetIris;
		sw = ui->stackedWidgetIris;
	}
	QListWidgetItem *item = new QListWidgetItem(deviceName);
	item->setTextAlignment(Qt::AlignCenter);
	lw->insertItem(lw->count(), item);
	ContentPane *contentPane = new ContentPane(deviceInfo);
	sw->addWidget(contentPane);
	contentPaneMap.insert(deviceName, contentPane);
	connect(this, &MainWindow::selectedUserChanged,
			contentPane, &ContentPane::setSelectedUser);
	connect(lw, &QListWidget::currentRowChanged, sw, &QStackedWidget::setCurrentIndex);
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
	for (QString deviceName: deviceInfoMap.keys())
		addContentPane(deviceName);
	checkBiometricPage(Fingerprint);
	checkBiometricPage(Fingervein);
	checkBiometricPage(Iris);
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
void MainWindow::initDashboardDeviceSection()
{
	ToggleSwitch *toggleSwitch;

	QString(tr("Device Name")); /* For SET_TABLE_ATTRIBUTE translation */
	QString(tr("Status"));
	SET_TABLE_ATTRIBUTE(ui->tableWidgetFingerprint);
	SET_TABLE_ATTRIBUTE(ui->tableWidgetFingervein);
	SET_TABLE_ATTRIBUTE(ui->tableWidgetIris);
	for (QString deviceName: deviceInfoMap.keys()) {
		DeviceInfo *deviceInfo = deviceInfoMap.value(deviceName);
		QTableWidget *tw = NULL;
		if (deviceInfo->biotype == BIOTYPE_FINGERPRINT)
			tw = ui->tableWidgetFingerprint;
		else if (deviceInfo->biotype == BIOTYPE_FINGERVEIN)
			tw = ui->tableWidgetFingervein;
		else if (deviceInfo->biotype == BIOTYPE_IRIS)
			tw = ui->tableWidgetIris;

		bool deviceAvailable = deviceInfoMap.value(deviceName)->device_available;
		toggleSwitch = new ToggleSwitch(deviceAvailable, DEVICE_TS_W, DEVICE_TS_H);
		connect(toggleSwitch, &ToggleSwitch::toggled, this, &MainWindow::manageDeviceStatus);

		/* 0 - column */
		int new_index = tw->rowCount();
		tw->insertRow(new_index);
		QTableWidgetItem *item = new QTableWidgetItem(deviceName);
		item->setFlags(item->flags() ^ Qt::ItemIsEditable);
		tw->setItem(new_index, 0, item);

		/* 1 - column */
		QWidget *cellAlignWidget = new QWidget();
		QVBoxLayout *vbox = new QVBoxLayout();
		vbox->addWidget(toggleSwitch);
		vbox->setContentsMargins(3, 2, 0, 0); /* center in vertical and horizontal */
		cellAlignWidget->setLayout(vbox);
		tw->setCellWidget(new_index, 1, cellAlignWidget);
	}
}

void MainWindow::initDashboardBioAuthSection()
{
	ToggleSwitch *toggleSwitch;
	QProcess process;
	process.start("bioctl status");
	process.waitForFinished();
	QString output = process.readAllStandardOutput();
	if (output.contains("enable", Qt::CaseInsensitive))
		toggleSwitch = new ToggleSwitch(true);
	else
		toggleSwitch = new ToggleSwitch(false);
	ui->horizontalLayoutBioAuth->addWidget(toggleSwitch);
	ui->horizontalLayoutBioAuth->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
	connect(toggleSwitch, &ToggleSwitch::toggled, this, &MainWindow::manageBioAuthStatus);
}

void MainWindow::manageDeviceStatus(bool toState)
{
	ToggleSwitch *toggleSwitch = (ToggleSwitch *)sender();
	QWidget *cellAlignWidget = toggleSwitch->parentWidget();
	QTableWidget *tableWidget = (QTableWidget *)(cellAlignWidget->parentWidget()->parentWidget());
	QTableWidgetItem *deviceNameItem;
	for (int i = 0; i < tableWidget->rowCount(); i++) {
		if (tableWidget->cellWidget(i, 1) == cellAlignWidget) {
			deviceNameItem = tableWidget->item(i, 0);
			break;
		}
	}
	QString deviceName = deviceNameItem->text();
	QProcess process;
	if (toState) {
		process.start("pkexec sh -c \"biometric-config-tool enable-driver "
				+ deviceName
				+ " && systemctl restart biometric-authentication.service");
		process.waitForFinished();
	} else {
		process.start("pkexec sh -c \"biometric-config-tool disable-driver "
				+ deviceName
				+ " && systemctl restart biometric-authentication.service");
		process.waitForFinished();
	}
	if (process.exitCode() != 0) {
		QMessageBox *messageBox = new QMessageBox(QMessageBox::Critical,
							tr("Fatal Error"),
							tr("Fail to change device status"),
							QMessageBox::Ok);
		messageBox->exec();
		return;
	}
	/*
	 * There is a condition that the driver is enabled while the device is
	 * not connected. So, if user enables the driver, we need to update the
	 * deviceinfo array to make sure the device is indeed available. If user
	 * disabled the driver, the device must can't be used and therefor we
	 * don't need to query device info from DBus.
	 */
	ContentPane *contentPane = contentPaneMap.value(deviceName);
	if (toState) {
		getDeviceInfo();
		bool deviceAvailable = deviceInfoMap.value(deviceName)->device_available;
		contentPane->setDeviceAvailable(deviceAvailable);
		if (deviceAvailable) {
			toggleSwitch->acceptStateChange();
		} else {
			QMessageBox *messageBox = new QMessageBox(QMessageBox::Critical,
							tr("Fatal Error"),
							tr("Device is not connected"),
							QMessageBox::Ok);
			messageBox->exec();
		}

	} else {
		deviceInfoMap.value(deviceName)->driver_enable = false;
		deviceInfoMap.value(deviceName)->device_available = false;
		contentPane->setDeviceAvailable(false);
		toggleSwitch->acceptStateChange();
	}
}

void MainWindow::manageBioAuthStatus(bool toState)
{
	ToggleSwitch *toggleSwitch = (ToggleSwitch *)sender();
	QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
	QProcess process;
	if (toState) {
		process.start("pkexec bioctl enable -u " + environment.value("USER"));
		process.waitForFinished();
		if (process.exitCode() == 0)
			toggleSwitch->acceptStateChange();
	} else {
		process.start("pkexec bioctl disable -u " + environment.value("USER"));
		process.waitForFinished();
		if (process.exitCode() == 0)
			toggleSwitch->acceptStateChange();
	}
}
