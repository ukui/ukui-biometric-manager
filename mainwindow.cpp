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
#include <driverinputdialog.h>
#include <QMessageBox>

#define ICON_SIZE 32
#define DRIVER_TS_W 45 /* The width of ToggleSwitch in driver list */
#define DRIVER_TS_H 26

MainWindow::MainWindow(QString usernameFromCmd, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	usernameFromCmd(usernameFromCmd)
{
	ui->setupUi(this);
	prettify();

	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
	biometricInterface = new cn::kylinos::Biometric("cn.kylinos.Biometric",
				"/cn/kylinos/Biometric",
				QDBusConnection::systemBus(), this);
	biometricInterface->setTimeout(2147483647); /* 微秒 */

	/* 获取设备列表 */
	getDeviceInfo();

	/* 获取并显示用户列表 */
	showUserList();

	/* Other initializations */
	dashboardPageInit();
	biometricPageInit();
	clearNoDevicePage();

	setDefaultUser();
}

MainWindow::~MainWindow()
{
	delete ui;
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
		driverCount = 0;
		return;
	}

	/* 解析 DBus 返回值，reply 有两个返回值，都是 QVariant 类型 */
	variant = reply.argumentAt(0); /* 得到第一个返回值 */
	driverCount = variant.value<int>(); /* 解封装得到设备个数 */
	variant = reply.argumentAt(1); /* 得到第二个返回值 */
	argument = variant.value<QDBusArgument>(); /* 解封装，获取QDBusArgument对象 */
	argument >> qlist; /* 使用运算符重载提取 argument 对象里面存储的列表对象 */

	deviceInfoMap.clear();
	for (int i = 0; i < driverCount; i++) {
		item = qlist[i]; /* 取出一个元素 */
		variant = item.variant(); /* 转为普通QVariant对象 */
		/* 解封装得到 QDBusArgument 对象 */
		argument = variant.value<QDBusArgument>();
		deviceInfo = new DeviceInfo();
		argument >> *deviceInfo; /* 提取最终的 DeviceInfo 结构体 */
		deviceInfoMap.insert(deviceInfo->device_shortname, deviceInfo);
	}

	/*
	 * The D-Bus result doesn't have the field device_available because the
	 * backend hasn't implemented it. We use the following function to set it
	 * manually. If the backend completes it implements, we can remove this
	 * function directly and then modify the customtype.cpp to use >> operator
	 * to do that.
	 */
	tmpSetDeviceAvailable();
}

void MainWindow::tmpSetDeviceAvailable()
{
	QVariant variant;
	QDBusArgument argument;
	QList<QDBusVariant> qlist;
	QDBusVariant item;
	DeviceInfo *availableDeviceInfo;

	QDBusPendingReply<int, QList<QDBusVariant> > reply = biometricInterface->GetDevList();
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}

	variant = reply.argumentAt(0);
	int availableDeviceCount = variant.value<int>();
	variant = reply.argumentAt(1);
	argument = variant.value<QDBusArgument>();
	argument >> qlist;

	for (DeviceInfo *info: deviceInfoMap.values())
		info->device_available = 0;

	for (int i = 0; i < availableDeviceCount; i++) {
		item = qlist[i];
		variant = item.variant();
		argument = variant.value<QDBusArgument>();
		availableDeviceInfo = new DeviceInfo();
		argument >> *availableDeviceInfo;
		deviceInfoMap.value(availableDeviceInfo->device_shortname)->device_available = 1;
	}
}

void MainWindow::addContentPane(QString driverName)
{
	QListWidget *lw;
	QStackedWidget *sw;
	DeviceInfo *deviceInfo = deviceInfoMap.value(driverName);
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
	QListWidgetItem *item = new QListWidgetItem(driverName);
	item->setTextAlignment(Qt::AlignCenter);
	lw->insertItem(lw->count(), item);
	ContentPane *contentPane = new ContentPane(deviceInfo);
	sw->addWidget(contentPane);
	contentPaneMap.insert(driverName, contentPane);
	connect(this, &MainWindow::selectedUserChanged,
			contentPane, &ContentPane::setSelectedUser);
	connect(lw, &QListWidget::currentRowChanged, sw, &QStackedWidget::setCurrentIndex);
}

void MainWindow::removeContentPane(QString driverName)
{
	QListWidget *lw;
	QStackedWidget *sw;
	DeviceInfo *deviceInfo = deviceInfoMap.value(driverName);
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
	int index = sw->indexOf(contentPaneMap.value(driverName));
	lw->removeItemWidget(lw->item(index));
	sw->removeWidget(contentPaneMap.value(driverName));
}

void MainWindow::biometricPageInit()
{
	for (QString driverName: deviceInfoMap.keys())
		addContentPane(driverName);
}

#define checkBiometricPage(biometric) do {				\
	if (ui->listWidget##biometric->count() >= 1) {			\
		ui->listWidget##biometric->setCurrentRow(0);		\
	} else {							\
		delete ui->listWidget##biometric;			\
		delete ui->stackedWidget##biometric;			\
		QLabel *lblNoDevice = new QLabel(tr("No Any Device"));	\
		lblNoDevice->setObjectName("lblNoDevice"#biometric);	\
		ui->horizontalLayout##biometric->addStretch();		\
		ui->horizontalLayout##biometric->addWidget(lblNoDevice);\
		ui->horizontalLayout##biometric->addStretch();		\
	}								\
} while(0)

void MainWindow::clearNoDevicePage()
{
	checkBiometricPage(Fingerprint);
	checkBiometricPage(Fingervein);
	checkBiometricPage(Iris);
}

QString MainWindow::mapReadableDeviceName(QString driverName)
{
	DeviceInfo *deviceInfo = deviceInfoMap.value(driverName);
	if (deviceInfo->biotype == BIOTYPE_FINGERPRINT)
		return QString(tr("Fingerprint"));
	else if (deviceInfo->biotype == BIOTYPE_FINGERVEIN)
		return QString(tr("Fingervein"));
	else
		return QString(tr("Iris"));
}

void MainWindow::dashboardPageInit()
{
	dashboardSystemdSection();
	dashboardDriverSection();
	dashboardBioAuthSection();
}

void MainWindow::dashboardSystemdSection()
{
	ToggleSwitch *toggleSwitch;
	QProcess process;
	process.start("systemctl is-active biometric-authentication.service");
	process.waitForFinished();
	QString output = process.readAllStandardOutput();
	if (output.startsWith("active"))
		toggleSwitch = new ToggleSwitch(true);
	else
		toggleSwitch = new ToggleSwitch(false);
	QHBoxLayout *hBoxLayout = (QHBoxLayout *)ui->groupBoxService->layout();
	QPushButton *btnRestartService = new QPushButton();
	btnRestartService->setIcon(QIcon(":/images/assets/restart.png"));
	btnRestartService->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	btnRestartService->setObjectName("btnRestartService");
	hBoxLayout->addWidget(toggleSwitch);
	hBoxLayout->addWidget(btnRestartService);
	hBoxLayout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum));
	connect(toggleSwitch, &ToggleSwitch::toggled, this, &MainWindow::manageServiceStatus);
	connect(btnRestartService, &QPushButton::clicked, this, &MainWindow::restartService);
}

void MainWindow::dashboardDriverSection()
{
	ToggleSwitch *toggleSwitch;
	QGridLayout *gridLayout = (QGridLayout *)ui->scrollAreaWidgetContents->layout();
	gridLayout->setAlignment(Qt::AlignTop);
	int i = 0;
	for (QString driverName: deviceInfoMap.keys()) {
		bool driverEnable = deviceInfoMap.value(driverName)->driver_enable;
		if (driverEnable)
			toggleSwitch = new ToggleSwitch(true, DRIVER_TS_W, DRIVER_TS_H);
		else
			toggleSwitch = new ToggleSwitch(false, DRIVER_TS_W, DRIVER_TS_H);
		connect(toggleSwitch, &ToggleSwitch::toggled, this, &MainWindow::manageDriverStatus);
		gridLayout->addWidget(new QLabel(driverName), i, 0);
		gridLayout->addWidget(new QLabel(mapReadableDeviceName(driverName)), i, 1);
		gridLayout->addWidget(toggleSwitch, i, 2);

		QPushButton *btnRemoveDriver = new QPushButton();
		btnRemoveDriver->setObjectName("btnRemoveDriver");
		btnRemoveDriver->setIcon(QIcon(":/images/assets/remove.png"));
		btnRemoveDriver->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
		gridLayout->addWidget(btnRemoveDriver, i, 3);
		connect(btnRemoveDriver, &QPushButton::clicked, this, &MainWindow::removeDriver);
		btnRemoveDriver->hide(); /* Temporarily hide this feature */
		i++;
	}
	/* Resize the column width automatically */
	gridLayout->setColumnStretch(0, 1);
	gridLayout->setColumnStretch(1, 1);

	ui->btnAddDriver->setIcon(QIcon(":/images/assets/add.png"));
	ui->btnAddDriver->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	connect(ui->btnAddDriver, &QPushButton::clicked, this, &MainWindow::addDriver);
	ui->btnAddDriver->hide(); /* Temporarily hide this feature */
}

void MainWindow::dashboardBioAuthSection()
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

void MainWindow::manageServiceStatus(bool toState)
{
	ToggleSwitch *toggleSwitch = (ToggleSwitch *)sender();
	QProcess process;
	if (toState) {
		process.start("systemctl start biometric-authentication.service");
		process.waitForFinished();
		if (process.exitCode() == 0)
			toggleSwitch->acceptStateChange();
	} else {
		process.start("systemctl stop biometric-authentication.service");
		process.waitForFinished();
		if (process.exitCode() == 0)
			toggleSwitch->acceptStateChange();
	}
}

void MainWindow::restartService()
{
	QProcess process;
	process.start("systemctl restart biometric-authentication.service");
	process.waitForFinished();
}

void MainWindow::manageDriverStatus(bool toState)
{
	ToggleSwitch *toggleSwitch = (ToggleSwitch *)sender();
	QGridLayout *gridLayout = (QGridLayout *)ui->scrollAreaWidgetContents->layout();
	int index = gridLayout->indexOf(toggleSwitch);
	QLabel *label = (QLabel *)gridLayout->itemAt(index-2)->widget(); /* Relative to toggleSwitch */
	QProcess process;
	QString driverName = label->text();
	if (toState) {
		process.start("pkexec sh -c \"biometric-config-tool enable-driver "
				+ driverName
				+ " && systemctl restart biometric-authentication.service");
		process.waitForFinished();
	} else {
		process.start("pkexec sh -c \"biometric-config-tool disable-driver "
				+ driverName
				+ " && systemctl restart biometric-authentication.service");
		process.waitForFinished();
	}
	if (process.exitCode() != 0)
		return;
	toggleSwitch->acceptStateChange();
	/*
	 * There is a condition that the driver is enabled while the device is
	 * not connected. So, if user enables the driver, we need to update the
	 * deviceinfo array to make sure the device is indeed available. If user
	 * disabled the driver, the device must can't be used and therefor we
	 * don't need to update the deviceinfo array.
	 */
	ContentPane *contentPane = contentPaneMap.value(driverName);
	contentPane->setDriverEnable(toState);
	if (toState) {
		getDeviceInfo();
		contentPane->setDeviceAvailable(deviceInfoMap.value(driverName)->device_available);
	} else {
		contentPane->setDeviceAvailable(false);
	}
}

void MainWindow::addDriver()
{
	DriverInputDialog *dialog = new DriverInputDialog();
	connect(dialog, &DriverInputDialog::inputCompleted, this, &MainWindow::addDriverCallback);
	dialog->exec();
}

void MainWindow::addDriverCallback(QString driverName, bool status, QString driverPath, QString devicePath)
{
	QProcess process;
	process.start("pkexec biometric-config-tool add-driver " + driverName + " " + driverPath);
	process.waitForFinished();
	if (process.exitCode() != 0) {
		QMessageBox *messageBox = new QMessageBox(QMessageBox::Information,
							tr("Error"),
							tr("Fail to add driver"),
							QMessageBox::Ok);
		messageBox->exec();
		return;
	}
	if (devicePath != "")
		process.start("pkexec biometric-config-tool set-key "
					+ driverName + " Path " + driverPath);
	process.waitForFinished();
	if (!status)
		process.start("pkexec biometric-config-tool disable-driver " + driverName);
	process.waitForFinished();

	QGridLayout *gridLayout = (QGridLayout *)ui->scrollAreaWidgetContents->layout();
	int rowCount = gridLayout->rowCount();
	ToggleSwitch *toggleSwitch = new ToggleSwitch(status, DRIVER_TS_W, DRIVER_TS_H);
	connect(toggleSwitch, &ToggleSwitch::toggled, this, &MainWindow::manageDriverStatus);
	gridLayout->addWidget(new QLabel(driverName), rowCount, 0);
	gridLayout->addWidget(toggleSwitch, rowCount, 1);

	QPushButton *btnRemoveDriver = new QPushButton();
	btnRemoveDriver->setObjectName("btnRemoveDriver");
	btnRemoveDriver->setIcon(QIcon(":/images/assets/remove.png"));
	btnRemoveDriver->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	gridLayout->addWidget(btnRemoveDriver, rowCount, 2);
	connect(btnRemoveDriver, &QPushButton::clicked, this, &MainWindow::removeDriver);

	/* https://stackoverflow.com/a/23978797/4112667 */
	int maximum = ui->scrollAreaDriver->verticalScrollBar()->maximum();
	maximum += ICON_SIZE; /* Increase maximum manually */
	ui->scrollAreaDriver->verticalScrollBar()->setMaximum(maximum);
	ui->scrollAreaDriver->verticalScrollBar()->setValue(maximum);

	/* Add corresponding contentPane to biometric page */
	restartService(); /* Make the addition take effect */
	getDeviceInfo(); /* Refresh */
	addContentPane(driverName);
}

void MainWindow::removeDriver()
{
	QPushButton *btnRemoveDriver = (QPushButton *)sender();
	QGridLayout *gridLayout = (QGridLayout *)ui->scrollAreaWidgetContents->layout();
	int index = gridLayout->indexOf(btnRemoveDriver);

	QLabel *lbl = (QLabel *)gridLayout->itemAt(index-3)->widget(); /* Relative to btnRemoveDriver */
	QString driverName = lbl->text();
	QProcess process;
	process.start("pkexec biometric-config-tool remove-driver " + driverName);
	process.waitForFinished();
	if (process.exitCode() != 0) {
		QMessageBox *messageBox = new QMessageBox(QMessageBox::Information,
							tr("Error"),
							tr("Fail to remove driver"),
							QMessageBox::Ok);
		messageBox->exec();
		return;
	}

	/* Remove table row */
	QWidget *widget;
	for (int i = 1; i <= 3; i++) {
		widget = gridLayout->itemAt(index-3)->widget(); /* Relative to btnRemoveDriver */
		gridLayout->removeWidget(widget);
		widget->deleteLater();
		delete widget;
	}

	/* Remove corresponding contentPane from biometric page */
	removeContentPane(driverName);
	restartService();
	deviceInfoMap.remove(driverName);
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
