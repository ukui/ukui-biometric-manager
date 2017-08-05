#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDBusArgument>
#include "customtype.h"
#include <QTimer>
#include <QInputDialog>
#include "promptdialog.h"

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
	biometricInterface->setTimeout(6000000);
	/* 获取设备列表 */
	getDeviceInfo();
	/* 获取所有设备已经使用的生物特征 index 列表 */
	trackAllBiometricIndex();
	/* 所有标签页初始情况下都处于未被展示过的状态，都需要在切换标签页时被展示 */
	pageFirstShow[0] = pageFirstShow[1] = pageFirstShow[2] = true;
	/* 设置数据模型 */
	setModel();
	/* 界面初开的设备类型 */
	currentBiotype = BIOTYPE_FINGERVEIN;
	/* 初始化计时器供后面的进度显示弹窗使用 */
	timer = new QTimer();
	/* 获取并显示用户列表, 也会触发 showBiometrics，需要在 model 初始化后才能调用  */
	showUserList();
}

MainWindow::~MainWindow()
{
	delete ui;
}

/**
 * @brief 设置数据模型
 */
void MainWindow::setModel()
{
	/* 设置 TreeView 的 Model */
	QStandardItemModel *modelFingerprint = new QStandardItemModel(ui->treeViewFingerprint);
	QStandardItemModel *modelFingervein = new QStandardItemModel(ui->treeViewFingervein);
	QStandardItemModel *modelIris = new QStandardItemModel(ui->treeViewIris);
	ui->treeViewFingerprint->setModel(modelFingerprint);
	ui->treeViewFingervein->setModel(modelFingervein);
	ui->treeViewIris->setModel(modelIris);
	modelFingerprint->setHorizontalHeaderLabels(
			QStringList() << QString("特征名称") << QString("index"));
	modelFingervein->setHorizontalHeaderLabels(
			QStringList() << QString("特征名称") << QString("index"));
	modelIris->setHorizontalHeaderLabels(
			QStringList() << QString("特征名称") << QString("index"));
	ui->treeViewFingerprint->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->treeViewFingervein->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->treeViewIris->setEditTriggers(QAbstractItemView::NoEditTriggers);
	modelMap.insert(BIOTYPE_FINGERPRINT, modelFingerprint);
	modelMap.insert(BIOTYPE_FINGERVEIN, modelFingervein);
	modelMap.insert(BIOTYPE_IRIS, modelIris);
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
		enum BioType biotype = static_cast<enum BioType>(deviceInfo->biotype);
		deviceInfoMap.insert(biotype, deviceInfo);
	}
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
		qDebug() << "GUI:" << ":";
	}

	QTextStream in(&file);

	while(!in.atEnd()) {
		line = in.readLine();
		fields = line.split(":");
		uname = fields[0];
		uid = fields[2].toInt();
		if (uid == 65534) /* nobody 用户 */
			continue;
		if (uid >=1000 || uid == 0)
			/* 这里会触发 currentIndexChanged 信号 */
			ui->comboBoxUname->addItem(uname, QVariant(uid));
	}
}

/**
 * @brief 获取所有设备已经被使用的生物特征 index 列表
 */
void MainWindow::trackAllBiometricIndex()
{
	QList<QDBusVariant> qlist;
	BiometricInfo *biometricInfo;
	QList<int> *indexList;
	int listsize;

	for (int i = BIOTYPE_FINGERPRINT; i <= __MAX_NR_BIOTYPES; i++) {
		enum BioType biotype = static_cast<enum BioType>(i);
		if (!deviceIsEnable(biotype))
			continue;
		QDBusPendingReply<int, QList<QDBusVariant> > reply =
				biometricInterface->GetFeatureList(
					deviceInfoMap.value(biotype)->driver_id,
					-1, 0, -1);
		reply.waitForFinished();
		if (reply.isError()) {
			qDebug() << "GUI:" << reply.error();
			return;
		}

		listsize = reply.argumentAt(0).value<int>();
		reply.argumentAt(1).value<QDBusArgument>() >> qlist;

		indexList = new QList<int>;
		for (int j = 0; j < listsize; j++) {
			biometricInfo = new BiometricInfo();
			qlist[j].variant().value<QDBusArgument>() >> *biometricInfo;
			indexList->append(biometricInfo->index);
		}
		qSort(*indexList);
		biometricIndexMap.insert(biotype, indexList);
	}
}

/**
 * @brief binary_search 二分查找
 * @param indexList
 * @param left
 * @param right
 * @return -1 表示本段连续，否则就是空隙的起始下标
 */
int binary_search(QList<int> &indexList, int left, int right)
{
	int mid;
	int boundaryPos;
	/* 本段连续 */
	if ((indexList[right] - indexList[left]) == right - left)
		return -1;
	/* 空闲位置搜索完成 */
	if ((right - left) == 1)
		return left;

	mid = (left + right)/2;
	boundaryPos = binary_search(indexList, left, mid);
	if (boundaryPos != -1)
		return boundaryPos;
	return binary_search(indexList, mid, right);
}
/**
 * @brief 查找一个空闲的特征 index
 * @return
 */
int MainWindow::findFreeBiometricIndex()
{
	QList<int> *indexList;
	indexList = biometricIndexMap.value(currentBiotype);
	int freeIndex, boundaryPos;
	if (indexList->isEmpty()){
		freeIndex = 1; /* 特征从1开始使用 */
		indexList->append(freeIndex);
		return freeIndex;
	}
	/* boundaryPos 是间隙的前一个元素的下标 */
	boundaryPos = binary_search(*indexList, 0, indexList->length()-1);
	if (boundaryPos == -1) /* 整个列表都是连续的 */
		boundaryPos = indexList->length() -1;
	freeIndex = indexList->at(boundaryPos) + 1;
	/* 将空闲index插入追踪列表 */
	freeIndexPos = boundaryPos + 1;
	indexList->insert(freeIndexPos, freeIndex);

	return freeIndex;
}

/**
 * @brief 检测某种生物识别设备是否存在
 * @param biotype
 * @return
 */
bool MainWindow::deviceIsEnable(enum BioType biotype)
{
	if (!deviceInfoMap.contains(biotype))
		return false; /* 设备不存在(未连接) */
	return deviceInfoMap.value(biotype)->enable;
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
		deviceEnable = deviceIsEnable(currentBiotype);
		ui->pageFingerprint->setEnabled(deviceEnable);
		break;
	case 1:
		currentBiotype = BIOTYPE_FINGERVEIN;
		deviceEnable = deviceIsEnable(currentBiotype);
		ui->pageFingervein->setEnabled(deviceEnable);
		break;
	case 2:
		currentBiotype = BIOTYPE_IRIS;
		deviceEnable = deviceIsEnable(currentBiotype);
		ui->pageIris->setEnabled(deviceEnable);
		break;
	default:
		qDebug() << "GUI:" << "tabWidget page index = -1";
		break;
	}

	setWidgetsEnabled(deviceEnable);
	showBiometrics();
}

/**
 * @brief 切换 ComboBox 的 Slot
 * @param index
 */
void MainWindow::on_comboBoxUname_currentIndexChanged(int index)
{
	qDebug() << "GUI:" << "ComboBox Changed";
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
	QList<QVariant> args;

	if (!deviceIsEnable(currentBiotype))
		return;
	/* 不能用clear()，它会将表头也清掉 */
	modelMap.value(currentBiotype)->setRowCount(0);

	args << QVariant(deviceInfoMap.value(currentBiotype)->driver_id)
		<< QVariant(currentUid) << QVariant(0) << QVariant(-1);
	biometricInterface->callWithCallback("GetFeatureList", args, this,
						SLOT(showBiometricsCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
}

/**
 * @brief 特征列表返回后回调函数进行显示
 * @param callbackReply
 */
void MainWindow::showBiometricsCallback(QDBusMessage callbackReply)
{
	QList<QDBusVariant> qlist;
	BiometricInfo *biometricInfo;
	int listsize;
	QList<QVariant> variantList = callbackReply.arguments();
	listsize = variantList[0].value<int>();
	variantList[1].value<QDBusArgument>() >> qlist;
	for (int i = 0; i < listsize; i++) {
		biometricInfo = new BiometricInfo();
		qlist[i].variant().value<QDBusArgument>() >> *biometricInfo;
		QList<QStandardItem *> row;
		row.append(new QStandardItem(biometricInfo->index_name));
		row.append(new QStandardItem(QString::number(biometricInfo->index)));
		modelMap.value(currentBiotype)->appendRow(row);
	}
}

/**
 * @brief 集中设置部分操作控件的激活状态
 * @param status
 */
void MainWindow::setWidgetsEnabled(bool status)
{
	ui->comboBoxUname->setEnabled(status);
	ui->btnAdd->setEnabled(status);
	ui->btnDelete->setEnabled(status);
	ui->btnDrop->setEnabled(status);
	ui->btnVerify->setEnabled(status);
}

/**
 * @brief 录入
 */
void MainWindow::on_btnAdd_clicked()
{
	int free_index = findFreeBiometricIndex();
	QList<QVariant> args;
	bool ok;
	QString text = QInputDialog::getText(this, "请输入特征名称",
					"特征名称", QLineEdit::Normal,
					"", &ok);
	if (!ok || text.isEmpty())
		return;
	indexName = text;
	/*
	 * 异步回调参考资料：
	 * https://github.com/RalfVB/SQEW-OS/blob/master/src/module/windowmanager/compton.cpp
	 */
	args << QVariant(deviceInfoMap.value(currentBiotype)->driver_id)
		<< QVariant(currentUid) << QVariant(free_index) << QVariant("xxx");
	biometricInterface->callWithCallback("Enroll", args, this,
						SLOT(dbusCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog = new PromptDialog(this);
	promptDialog->onlyShowCancle();
	/*
	connect(promptDialog, &PromptDialog::rejected, [this]{
						this->canceledByUser=true;
						});*/
	connect(promptDialog, &PromptDialog::canceled, this, &MainWindow::cancelOperation);
	connect(timer, &QTimer::timeout, this, &MainWindow::setOperationMsg);
	timer->start(600);
	promptDialog->exec();
	timer->stop();
}

/**
 * @brief DBus 异步回调函数
 */
void MainWindow::dbusCallback(QDBusMessage callbackReply)
{
	/*
	 * 该状态返回值指示了操作成功-0/操作失败-1/设备忙-2/没有设备-3
	 * 显示给用户的信息还是由定时器提供
	 */
	int dbusStatus, opsStatus;
	dbusStatus  = callbackReply.arguments()[0].value<int>();
	switch (dbusStatus) {
	case 0:
		qDebug() << "GUI:" << "操作成功";
		break;
	case 1:
		qDebug() << "GUI:" << "操作失败或用户取消";
		break;
	case 2:
		qDebug() << "GUI:" << "设备忙";
		break;
	case 3:
		qDebug() << "GUI:" << "设备不存在";
		break;
	default:
		break;
	}
	/* 读取本次设备操作的最终状态 */
	QDBusPendingReply<int, int, int, int, int> reply =
			biometricInterface->UpdateStatus(
				deviceInfoMap.value(currentBiotype)->driver_id);
	reply.waitForFinished();
	opsStatus = reply.argumentAt(3).value<int>();
	QList<QStandardItem *> row;
	switch (opsStatus%100) {
	case OPS_SUCCESS:
		row.append(new QStandardItem(indexName));
		row.append(new QStandardItem(QString::number(
			biometricIndexMap.value(currentBiotype)->at(freeIndexPos)
		)));
		modelMap.value(currentBiotype)->appendRow(row);
		break;
	case OPS_FAILED:
	case OPS_ERROR:
	case OPS_CANCEL:
	case OPS_TIMEOUT:
		biometricIndexMap.value(currentBiotype)->removeAt(freeIndexPos);
		break;
	}
	promptDialog->onlyShowOK();
}

/**
 * @brief DBus 异步操作 errorMethod
 * @param error
 */
void MainWindow::errorCallback(QDBusError error)
{
	qDebug() << "GUI:" << error.message();
}

/**
 * @brief 周期性设置弹窗的提示文字
 */
void MainWindow::setOperationMsg()
{
	QString msg;
	QDBusPendingReply<QString> reply =
			biometricInterface->GetNotifyMesg(
				deviceInfoMap.value(currentBiotype)->driver_id);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		promptDialog->setLabelText("读取操作信息失败");
		return;
	}

	msg = reply.argumentAt(0).value<QString>();
	promptDialog->setLabelText(msg);
}

/**
 * @brief 点击取消按钮时被间接触发
 */
void MainWindow::cancelOperation()
{
	QList<QVariant> args;
	timer->stop();
	args << QVariant(deviceInfoMap.value(currentBiotype)->driver_id)
		<< QVariant(5);
	biometricInterface->callWithCallback("StopOps", args, this,
						SLOT(cancelCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog->setLabelText("取消中...请稍后...");
}

/**
 * @brief 取消成功后的 DBus 回调函数
 * @param reply
 */
void MainWindow::cancelCallback(QDBusMessage callbackReply)
{
	promptDialog->closeDialog();
}
