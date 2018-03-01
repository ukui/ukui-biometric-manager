#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDBusArgument>
#include "customtype.h"
#include <QFile>
#include <QProcessEnvironment>
#include "contentpane.h"
#include "toggleswitch.h"
#include <unistd.h>

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
	setDefaultUser();

	/* Other initializations */
	dashboardPageInit();
	biometricPageInit();
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
	qApp->setStyleSheet(styleSheet);
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
	QDBusPendingReply<int, QList<QDBusVariant> > reply = biometricInterface->GetDevList();
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

	for (int i = 0; i < deviceCount; i++) {
		item = qlist[i]; /* 取出一个元素 */
		variant = item.variant(); /* 转为普通QVariant对象 */
		/* 解封装得到 QDBusArgument 对象 */
		argument = variant.value<QDBusArgument>();
		deviceInfo = new DeviceInfo();
		argument >> *deviceInfo; /* 提取最终的 DeviceInfo 结构体 */
		deviceInfoList[i] = deviceInfo;
	}
}

#define widgetAppendToTabPage(biometric) do {				\
	QListWidget *lw = ui->listWidget##biometric;			\
	QString sn = deviceInfoList[i]->device_shortname;		\
	QListWidgetItem *item = new QListWidgetItem(sn);		\
	item->setTextAlignment(Qt::AlignCenter);			\
	lw->insertItem(lw->count(), item);				\
	QStackedWidget *sw = ui->stackedWidget##biometric;		\
	ContentPane *contentPane = new ContentPane(deviceInfoList[i]);	\
	sw->addWidget(contentPane);					\
	connect(this, &MainWindow::selectedUserChanged,			\
			contentPane, &ContentPane::setSelectedUser);	\
} while(0)
#define widgetConnectSignal(biometric) do {				\
	connect(ui->listWidget##biometric, &QListWidget::currentRowChanged,\
		ui->stackedWidget##biometric, &QStackedWidget::setCurrentIndex);\
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
void MainWindow::biometricPageInit()
{
	for (int i = 0; i < deviceCount; i++) {
		switch (deviceInfoList[i]->biotype) {
		case BIOTYPE_FINGERPRINT:
			widgetAppendToTabPage(Fingerprint);
			break;
		case BIOTYPE_FINGERVEIN:
			widgetAppendToTabPage(Fingervein);
			break;
		case BIOTYPE_IRIS:
			widgetAppendToTabPage(Iris);
			break;
		}
	}
	widgetConnectSignal(Fingerprint);
	widgetConnectSignal(Fingervein);
	widgetConnectSignal(Iris);
}

void MainWindow::dashboardPageInit()
{
	ToggleSwitch *toggleSwitch;

	/* Systemd */
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

	/* Driver */
	QSettings settings(QString("/etc/biometric-auth/biometric-auth.conf"),
							QSettings::IniFormat);
	QStringList groups = settings.childGroups();
	QGridLayout *gridLayout = (QGridLayout *)ui->scrollAreaWidgetContents->layout();
	for (int i = 0; i < groups.count(); i++) {
		bool enable = settings.value(groups[i] + "/Enable").toBool();
		if (enable)
			toggleSwitch = new ToggleSwitch(true, DRIVER_TS_W, DRIVER_TS_H);
		else
			toggleSwitch = new ToggleSwitch(false, DRIVER_TS_W, DRIVER_TS_H);
		connect(toggleSwitch, &ToggleSwitch::toggled, this, &MainWindow::manageDriverStatus);
		gridLayout->addWidget(new QLabel(groups[i]), i, 0);
		gridLayout->addWidget(toggleSwitch, i, 1);

		QPushButton *btnRemoveDriver = new QPushButton();
		btnRemoveDriver->setObjectName("btnRemoveDriver");
		btnRemoveDriver->setIcon(QIcon(":/images/assets/remove.png"));
		btnRemoveDriver->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
		gridLayout->addWidget(btnRemoveDriver, i, 2);
	}
	ui->btnAddDriver->setIcon(QIcon(":/images/assets/add.png"));
	ui->btnAddDriver->setIconSize(QSize(ICON_SIZE, ICON_SIZE));

	/* Authentication */
	process.start("bioctl status");
	process.waitForFinished();
	output = process.readAllStandardOutput();
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
	QLabel *label = (QLabel *)gridLayout->itemAt(index - 1)->widget();
	QProcess process;
	QString configGroupName = label->text();
	if (toState) {
		process.start("pkexec biometric-config-tool enable-driver " + configGroupName);
		process.waitForFinished();
		if (process.exitCode() == 0)
			toggleSwitch->acceptStateChange();
	} else {
		process.start("pkexec biometric-config-tool disable-driver " + configGroupName);
		process.waitForFinished();
		if (process.exitCode() == 0)
			toggleSwitch->acceptStateChange();
	}
}

void MainWindow::manageBioAuthStatus(bool toState)
{
	ToggleSwitch *toggleSwitch = (ToggleSwitch *)sender();
	QProcess process;
	if (toState) {
		process.start("pkexec bioctl enable");
		process.waitForFinished();
		if (process.exitCode() == 0)
			toggleSwitch->acceptStateChange();
	} else {
		process.start("pkexec bioctl disable");
		process.waitForFinished();
		if (process.exitCode() == 0)
			toggleSwitch->acceptStateChange();
	}
}
