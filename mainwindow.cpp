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
	emit ui->tabWidget->currentChanged(1);
}

MainWindow::~MainWindow()
{
	delete ui;
}

/**
 * @brief 检测某种生物识别设备是否存在
 * @param biotype
 * @return
 */
bool MainWindow::deviceAvailable(enum BIOTYPE biotype)
{
	int count;
	QVariant variant;
	QDBusArgument argument;
	QList<QDBusVariant> qlist;
	DeviceInfo deviceInfo;

	/* 连接 DBus Daemon 并调用远程方法 */
	biometricInterface = new cn::kylinos::Biometric("cn.kylinos.Biometric",
				"/cn/kylinos/Biometric",
				QDBusConnection::systemBus(), this);
	/* 返回值为 i -- int 和 av -- array of variant */
	QDBusPendingReply<int, QList<QDBusVariant> > reply = biometricInterface->GetDevList();
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return false;
	}

	/* 解析 DBus 返回值 */
	variant = reply.argumentAt(0); /* 得到结果中的第一个参数 */
	count = variant.value<int>(); /* 解封装得到设备个数 */
	variant = reply.argumentAt(1); /* 得到结果中的第二个参数 */
	argument = variant.value<QDBusArgument>(); /* 解封装，获取QDBusArgument对象 */
	argument >> qlist; /* 使用运算符重载提取 argument 对象里面存储的列表对象 */

	for (int i = 0; i < count; i++) {
		QDBusVariant item = qlist[i]; /* 取出一个元素 */
		QVariant structVariant = item.variant(); /* 转为普通QVariant对象 */
		/* 解封装得到 QDBusArgument 对象 */
		QDBusArgument structArg = structVariant.value<QDBusArgument>();
		structArg >> deviceInfo; /* 提取最终的 DeviceInfo 结构体 */
		if (biotype == deviceInfo.biotype)
			return deviceInfo.enable;
	}

	/* 设备不存在 */
	return false;
}

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
