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
	/* 获取设备列表 */
	getDeviceInfoList();
	/* 触发信号以检测当前页面所对应的设备的可用性 */
	emit ui->tabWidget->currentChanged(1);
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
	DeviceInfo *deviceInfo;

	/* 连接 DBus Daemon 并调用远程方法 */
	biometricInterface = new cn::kylinos::Biometric("cn.kylinos.Biometric",
				"/cn/kylinos/Biometric",
				QDBusConnection::systemBus(), this);
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
		QDBusVariant item = qlist[i]; /* 取出一个元素 */
		variant = item.variant(); /* 转为普通QVariant对象 */
		/* 解封装得到 QDBusArgument 对象 */
		argument = variant.value<QDBusArgument>();
		deviceInfo = new DeviceInfo();
		argument >> *deviceInfo; /* 提取最终的 DeviceInfo 结构体 */
		deviceInfoList.append(deviceInfo);
	}
}

/**
 * @brief 检测某种生物识别设备是否存在
 * @param biotype
 * @return
 */
bool MainWindow::deviceAvailable(enum BIOTYPE biotype)
{
	for (int i = 0; i < deviceCount; i++) {
		if (biotype == deviceInfoList[i]->biotype)
			return deviceInfoList[i]->enable;
	}
	/* 设备不存在(未连接) */
	return false;
}

/**
 * @brief 切换标签页时检测当前页面对应的设备是否可用
 * @param pageIndex
 */
void MainWindow::on_tabWidget_currentChanged(int pageIndex)
{
	bool deviceEnable = false;
	if (pageIndex == 0) {
		deviceEnable = deviceAvailable(BIOTYPE_FINGERPRINT);
		ui->pageFingerprint->setEnabled(deviceEnable);
	} else if (pageIndex == 1) {
		deviceEnable = deviceAvailable(BIOTYPE_FINGERVEIN);
		ui->pageFingervein->setEnabled(deviceEnable);
	} else if (pageIndex == 2) {
		deviceEnable = deviceAvailable(BIOTYPE_IRIS);
		ui->pageIris->setEnabled(deviceEnable);
	} else {
		qDebug() << "GUI:" << "tabWidget page index = -1";
	}
}
