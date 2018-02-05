#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDBusArgument>
#include "customtype.h"
#include <QFile>
#include <QProcessEnvironment>
#include "contentpane.h"

MainWindow::MainWindow(QString usernameFromCmd, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow),
	usernameFromCmd(usernameFromCmd)
{
	ui->setupUi(this);
	/* 设置窗口图标 */
	QApplication::setWindowIcon(QIcon(":/images/assets/icon.png"));
	/* 设置 CSS */
	QFile qssFile(":/css/assets/mainwindow.qss");
	qssFile.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(qssFile.readAll());
	qApp->setStyleSheet(styleSheet);
	qssFile.close();

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


/**
 * @brief 获取并显示用户列表
 */
void MainWindow::showUserList()
{
	QFile file("/etc/passwd");
	QString line;
	QStringList fields;
	QString uname;
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
		uname = fields[0];
		uid = fields[2].toInt();
		if (uid == 65534) /* nobody 用户 */
			continue;
		if (uid >=1000 || uid == 0)
			ui->comboBoxUsername->addItem(uname, QVariant(uid));
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

void MainWindow::on_tabWidgetMain_currentChanged(int index)
{
	QObject *currentPage = ui->tabWidgetMain->widget(index);
	QString pageName = currentPage->objectName();
	QListWidget *lw;
	if (pageName == "pageDashboard")
		return;
	else if (pageName == "pageFingerprint")
		lw = ui->listWidgetFingerprint;
	else if (pageName == "pageFingervein")
		lw = ui->listWidgetFingervein;
	else if (pageName == "pageIris")
		lw = ui->listWidgetIris;
	if (lw->count() >= 1)
		lw->setCurrentRow(0);
}

#define widgetAppendToTabPage(biometric) do {				\
	QListWidget *lw = ui->listWidget##biometric;			\
	QString sn = deviceInfoList[i]->device_shortname;		\
	lw->insertItem(lw->count(), sn);				\
	QStackedWidget *sw = ui->stackedWidget##biometric;		\
	ContentPane *contentPane = new ContentPane(deviceInfoList[i]);	\
	sw->addWidget(contentPane);					\
	connect(this, &MainWindow::selectedUserChanged,			\
			contentPane, &ContentPane::setSelectedUser);	\
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
	connect(ui->listWidgetFingerprint, &QListWidget::currentRowChanged,
		this, &MainWindow::changeContentPane);
	connect(ui->listWidgetFingervein, &QListWidget::currentRowChanged,
		this, &MainWindow::changeContentPane);
	connect(ui->listWidgetIris, &QListWidget::currentRowChanged,
		this, &MainWindow::changeContentPane);
}

void MainWindow::changeContentPane(int index)
{
	QObject *senderObject = sender();
	QString senderName = senderObject->objectName();
	QStackedWidget *sw;
	if (senderName == "listWidgetFingerprint")
		sw = ui->stackedWidgetFingerprint;
	else if (senderName == "listWidgetFingervein")
		sw = ui->stackedWidgetFingervein;
	else if (senderName == "listWidgetIris")
		sw = ui->stackedWidgetIris;
	qDebug() << "GUI:" << "ContentPane Changed by" << senderName;
	/* 切换 ContentPane 并显示数据 */
	sw->setCurrentIndex(index);
	ContentPane *currentContentPane = (ContentPane *)sw->widget(index);
	currentContentPane->showBiometrics();
}

/*
 * Set button status
 */
void MainWindow::setServiceButtonStatus(bool status)
{
	if (status) {
		ui->lblServiceStatus->setText(tr("Started"));
		ui->btnStartService->setEnabled(false);
		ui->btnStopService->setEnabled(true);
	} else {
		ui->lblServiceStatus->setText(tr("Stopped"));
		ui->btnStartService->setEnabled(true);
		ui->btnStopService->setEnabled(false);
	}
}

void MainWindow::setDriverButtonStatus(bool status)
{
	if (status) {
		ui->btnEnableDriver->setEnabled(false);
		ui->btnDisableDriver->setEnabled(true);
	} else {
		ui->btnEnableDriver->setEnabled(true);
		ui->btnDisableDriver->setEnabled(false);
	}
}

void MainWindow::setBioAuthButtonStatus(bool status)
{
	if (status) {
		ui->lblBioAuthStatus->setText(tr("Enabled"));
		ui->btnEnableBioAuth->setEnabled(false);
		ui->btnDisableBioAuth->setEnabled(true);
	} else {
		ui->lblBioAuthStatus->setText(tr("Disabled"));
		ui->btnEnableBioAuth->setEnabled(true);
		ui->btnDisableBioAuth->setEnabled(false);
	}
}

void MainWindow::dashboardPageInit()
{
	/* Systemd */
	QProcess process;
	process.start("systemctl is-active biometric-authentication.service");
	process.waitForFinished();
	QString output = process.readAllStandardOutput();
	if (output.contains("active"))
		setServiceButtonStatus(true);
	else
		setServiceButtonStatus(false);

	/* Driver */
	QSettings settings(QString("/etc/biometric-auth/biometric-auth.conf"),
							QSettings::IniFormat);
	QStringList groups = settings.childGroups();
	ui->tableWidgetDriver->setRowCount(groups.count());
	ui->tableWidgetDriver->setColumnCount(3);
	ui->tableWidgetDriver->horizontalHeader()->setStretchLastSection(true);
	for (int i = 0; i < groups.count(); i++) {
		bool enable = settings.value(groups[i] + "/Enable").toBool();
		ui->tableWidgetDriver->setItem(i, 0, new QTableWidgetItem(groups[i]));
		if (enable)
			ui->tableWidgetDriver->setItem(i, 1, new QTableWidgetItem(tr("Enabled")));
		else
			ui->tableWidgetDriver->setItem(i, 1, new QTableWidgetItem(tr("Disabled")));
	}

	/* Authentication */
	process.start("bioctl status");
	process.waitForFinished();
	output = process.readAllStandardOutput();
	if (output.contains("enable", Qt::CaseInsensitive))
		setBioAuthButtonStatus(true);
	else
		setBioAuthButtonStatus(false);

	/* Signals */
	connect(ui->btnStartService, &QPushButton::clicked, this, &MainWindow::manageServiceStatus);
	connect(ui->btnStopService, &QPushButton::clicked, this, &MainWindow::manageServiceStatus);
	connect(ui->btnRestartService, &QPushButton::clicked, this, &MainWindow::manageServiceStatus);
}

void MainWindow::on_tableWidgetDriver_currentItemChanged(QTableWidgetItem *current, QTableWidgetItem *previous)
{
	UNUSED(current);
	UNUSED(previous);
	int currentIndex = ui->tableWidgetDriver->currentRow();
	QString status = ui->tableWidgetDriver->item(currentIndex, 1)->text();
	if (status == tr("Enabled"))
		setDriverButtonStatus(true);
	else
		setDriverButtonStatus(false);
}

void MainWindow::manageServiceStatus()
{
	QProcess process;
	QObject *senderObject = sender();
	QString senderName = senderObject->objectName();
	if (senderName == "btnStartService") {
		process.start("systemctl start biometric-authentication.service");
		process.waitForFinished();
		if (process.exitCode() == 0)
			setServiceButtonStatus(true);
	} else if (senderName == "btnStopService") {
		process.start("systemctl stop biometric-authentication.service");
		process.waitForFinished();
		if (process.exitCode() == 0)
			setServiceButtonStatus(false);
	} else if (senderName == "btnRestartService") {
		process.start("systemctl restart biometric-authentication.service");
		process.waitForFinished();
		if (process.exitCode() == 0)
			setServiceButtonStatus(true);
	}
}
