#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDBusArgument>
#include "customtype.h"
#include "devicespec.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
	biometricInterface = new cn::kylinos::Biometric("cn.kylinos.Biometric",
				"/cn/kylinos/Biometric",
				QDBusConnection::systemBus(), this);
	/* 获取设备列表 */
	getDeviceInfoList();
	/* 获取并显示用户列表 */
	getUserList();
	/* 所有标签页初始情况下都处于未被展示过的状态，都需要在切换标签页时被展示 */
	pageFirstShow[0] = pageFirstShow[1] = pageFirstShow[2] = true;
	/* 初始标签页 */
	emit ui->tabWidget->currentChanged(1);
	/* 设置 TreeView 的 Model */
	modelFingervein = new QStandardItemModel(ui->treeViewFingervein);
	ui->treeViewFingervein->setModel(modelFingervein);
	modelFingervein->setHorizontalHeaderLabels(
			QStringList() << QString("特征名称") << QString("index"));
	ui->treeViewFingervein->setEditTriggers(QAbstractItemView::NoEditTriggers);

}

MainWindow::~MainWindow()
{
	delete ui;
}

/**
 * @brief 获取设备列表并存储起来备用
 */
void MainWindow::getDeviceInfoList()
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
		enum BIOTYPE biotype = static_cast<enum BIOTYPE>(deviceInfo->biotype);
		deviceInfoMap.insert(biotype, deviceInfo);
	}
}

/**
 * @brief 获取并显示用户列表
 */
void MainWindow::getUserList()
{
	QFile file("/etc/passwd");
	QString line;
	QStringList fields;
	QString uname;
	int uid;

	if(!file.open(QIODevice::ReadOnly)) {
		qDebug() << "GUI:" << ":";
	}

	QTextStream in(&file);

	while(!in.atEnd()) {
		line = in.readLine();
		fields = line.split(":");
		uname = fields[0];
		uid = fields[2].toInt();
		if (uid >=1000 || uid == 0)
			ui->comboBoxUname->addItem(uname, QVariant(uid));
	}
}

/**
 * @brief 检测某种生物识别设备是否存在
 * @param biotype
 * @return
 */
bool MainWindow::deviceIsEnable()
{
	if (!deviceInfoMap.contains(currentBiotype))
		return false; /* 设备不存在(未连接) */
	return deviceInfoMap.value(currentBiotype)->enable;
}

/**
 * @brief 切换标签页的 Slot，检测当前页面对应的设备是否可用
 * @param pageIndex
 */
void MainWindow::on_tabWidget_currentChanged(int pageIndex)
{
	bool deviceEnable = false;
	switch (pageIndex) {
	case 0:
		currentBiotype = BIOTYPE_FINGERPRINT;
		deviceEnable = deviceIsEnable();
		ui->pageFingerprint->setEnabled(deviceEnable);
		ui->comboBoxUname->setEnabled(deviceEnable);
		break;
	case 1:
		currentBiotype = BIOTYPE_FINGERVEIN;
		deviceEnable = deviceIsEnable();
		ui->pageFingervein->setEnabled(deviceEnable);
		ui->comboBoxUname->setEnabled(deviceEnable);
		break;
	case 2:
		currentBiotype = BIOTYPE_IRIS;
		deviceEnable = deviceIsEnable();
		ui->pageIris->setEnabled(deviceEnable);
		ui->comboBoxUname->setEnabled(deviceEnable);
		break;
	default:
		qDebug() << "GUI:" << "tabWidget page index = -1";
		break;
	}
	/*
	 * 标签切换并不总是会造成数据刷新，只有在当前页面之前未曾显示过数据的情况下才会调用
	 * showBiometrics() 显示数据，此后再切换到此标签页上来，数据不再刷新，否则会拖慢速度。
	 * 另外，还要检测当前页面对应的设备可用是否可用，如果不可用，则不显示数据，
	 * pageFirstShow[pageIndex] 也不做改变
	 */
	if (pageFirstShow[pageIndex] && deviceEnable){
		showBiometrics();
		pageFirstShow[pageIndex] = false;
	}
}

/**
 * @brief 切换 ComboBox 的 Slot
 * @param index
 */
void MainWindow::on_comboBoxUname_activated(int index)
{
	int uid = ui->comboBoxUname->itemData(index).toInt();
	currentUid = uid;
	showBiometrics();
}

/**
 * @brief 显示生物识别数据列表
 * @param biotype
 */
void MainWindow::showBiometrics()
{
	QVariant variant;
	QDBusArgument argument;
	QList<QDBusVariant> qlist;
	QDBusVariant item;
	BiometricInfo *biometricInfo;
	int listsize;

	QDBusPendingReply<int, QList<QDBusVariant> > reply =
				biometricInterface->GetFeatureList(
					deviceInfoMap[currentBiotype]->driver_id,
					currentUid, 0, -1);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}

	variant = reply.argumentAt(0);
	listsize = variant.value<int>();
	variant = reply.argumentAt(1);
	argument = variant.value<QDBusArgument>();
	argument >> qlist;

	for (int i = 0; i < listsize; i++) {
		item = qlist[i]; /* 取出一个元素 */
		variant = item.variant(); /* 转为普通QVariant对象 */
		/* 解封装得到 QDBusArgument 对象 */
		argument = variant.value<QDBusArgument>();
		biometricInfo = new BiometricInfo();
		argument >> *biometricInfo; /* 提取最终的 BiometricInfo 结构体 */
		QList<QStandardItem *> row;
		row.append(new QStandardItem(biometricInfo->index_name));
		row.append(new QStandardItem(QString::number(biometricInfo->index)));
		modelFingervein->appendRow(row);
	}
}
