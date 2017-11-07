#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDBusArgument>
#include "customtype.h"
#include <QInputDialog>
#include <QFile>
#include <QProcessEnvironment>
#include "promptdialog.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	/* 设备类型与 GIF 的映射关系 */
	gifMap.insert(BIOTYPE_FINGERPRINT, ":/images/assets/fingerprint.gif");
	gifMap.insert(BIOTYPE_FINGERVEIN, ":/images/assets/fingervein.gif");
	gifMap.insert(BIOTYPE_IRIS, ":/images/assets/iris.gif");

	/* 设置窗口图标 */
	QApplication::setWindowIcon(QIcon(":/images/assets/icon.png"));
	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
	biometricInterface = new cn::kylinos::Biometric("cn.kylinos.Biometric",
				"/cn/kylinos/Biometric",
				QDBusConnection::systemBus(), this);
	biometricInterface->setTimeout(2147483647); /* 微秒 */
	/* 获取设备列表 */
	getDeviceInfo();
	/* 获取所有设备已经使用的生物特征 index 列表 */
	trackAllBiometricIndex();
	/* 设置数据模型 */
	setModel();
	/* 界面默认的设备类型为指纹设备 */
	currentBiotype = BIOTYPE_FINGERPRINT;
	/* 界面初次显示时不会触发标签页切换事件，所以要手动检测默认的指纹设备的可用性 */
	setWidgetsEnabled(deviceIsEnable(currentBiotype));
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
			QStringList() << QString(tr("Feature name")) << QString(tr("index")));
	modelFingervein->setHorizontalHeaderLabels(
			QStringList() << QString(tr("Feature name")) << QString(tr("index")));
	modelIris->setHorizontalHeaderLabels(
			QStringList() << QString(tr("Feature name")) << QString(tr("index")));
	ui->treeViewFingerprint->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->treeViewFingervein->setEditTriggers(QAbstractItemView::NoEditTriggers);
	ui->treeViewIris->setEditTriggers(QAbstractItemView::NoEditTriggers);
	/* 设备与数据模型的映射 */
	dataModelMap.insert(BIOTYPE_FINGERPRINT, modelFingerprint);
	dataModelMap.insert(BIOTYPE_FINGERVEIN, modelFingervein);
	dataModelMap.insert(BIOTYPE_IRIS, modelIris);
	/* 设备与 TreeView 的映射 */
	treeViewMap.insert(BIOTYPE_FINGERPRINT, ui->treeViewFingerprint);
	treeViewMap.insert(BIOTYPE_FINGERVEIN, ui->treeViewFingervein);
	treeViewMap.insert(BIOTYPE_IRIS, ui->treeViewIris);
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
		qDebug() << "GUI:" << "/etc/passwd can not be opened";
	}

	QTextStream in(&file);

	/* 阻止 addItem 触发 currentIndexChanged 信号 */
	ui->comboBoxUname->blockSignals(true);
	while(!in.atEnd()) {
		line = in.readLine();
		fields = line.split(":");
		uname = fields[0];
		uid = fields[2].toInt();
		if (uid == 65534) /* nobody 用户 */
			continue;
		if (uid >=1000 || uid == 0)
			ui->comboBoxUname->addItem(uname, QVariant(uid));
	}
	file.close();
	ui->comboBoxUname->blockSignals(false);
	/* 获取当前用户名 */
	QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
	/* 触发 currentIndexChanged 信号 */
	ui->comboBoxUname->setCurrentText(environment.value("USER"));
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
 * @return -1 表示本段连续，否则就是空隙的左边界下标
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
 * @return 特征 index 从 1 开始使用
 */
int findGap(QList<int> &indexList)
{
	int freeIndex, boundaryPos;
	/* 插入占位index,值为0,简化处理逻辑的同时也保证index从1开始利用，不会在前面出现断层 */
	indexList.prepend(0);
	/* boundaryPos 是空隙的左边界下标 */
	boundaryPos = binary_search(indexList, 0, indexList.length()-1);
	if (boundaryPos == -1) /* 整个列表都是连续的 */
		boundaryPos = indexList.length() -1;
	freeIndex = indexList[boundaryPos] + 1;
	/* 将空闲index插入追踪列表 */
	indexList.insert(boundaryPos + 1, freeIndex);
	/* 删除插入的占位元素 */
	indexList.removeFirst();

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
		break;
	case 1:
		currentBiotype = BIOTYPE_FINGERVEIN;
		break;
	case 2:
		currentBiotype = BIOTYPE_IRIS;
		break;
	default:
		qDebug() << "GUI:" << "tabWidget page index = -1";
		break;
	}

	deviceEnable = deviceIsEnable(currentBiotype);
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
	dataModelMap.value(currentBiotype)->setRowCount(0);

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
		dataModelMap.value(currentBiotype)->appendRow(row);
	}
}

/**
 * @brief 集中设置部分操作控件的激活状态
 * @param status
 */
void MainWindow::setWidgetsEnabled(bool status)
{
	treeViewMap.value(currentBiotype)->setEnabled(status);
	ui->comboBoxUname->setEnabled(status);
	ui->btnEnroll->setEnabled(status);
	ui->btnDelete->setEnabled(status);
	ui->btnDrop->setEnabled(status);
	ui->btnVerify->setEnabled(status);
	ui->btnSearch->setEnabled(status);
}

/**
 * @brief 录入
 */
void MainWindow::on_btnEnroll_clicked()
{
	QList<QVariant> args;
	bool ok;
	QString text = QInputDialog::getText(this, tr("Please input a feature name"),
					tr("Feature name"), QLineEdit::Normal,
					"", &ok);
	if (!ok || text.isEmpty())
		return;
	indexName = text;
	/* 查找空闲 index */
	QList<int> *indexList = biometricIndexMap.value(currentBiotype);
	freeIndex = findGap(*indexList);
	/*
	 * 异步回调参考资料：
	 * https://github.com/RalfVB/SQEW-OS/blob/master/src/module/windowmanager/compton.cpp
	 */
	args << QVariant(deviceInfoMap.value(currentBiotype)->driver_id)
		<< QVariant(currentUid) << QVariant(freeIndex) << QVariant(indexName);
	biometricInterface->callWithCallback("Enroll", args, this,
						SLOT(enrollCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog = new PromptDialog(gifMap.value(currentBiotype), this);
	promptDialog->onlyShowCancel();
	/*
	connect(promptDialog, &PromptDialog::rejected, [this]{
						this->canceledByUser=true;
						});*/
	connect(promptDialog, &PromptDialog::canceled, this, &MainWindow::cancelOperation);
	connect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
	/* 绑定第二个 SLOT，判别录入过程等待 Polkit 授权的阶段，输出特殊提示覆盖 Polkit 搜索操作造成的搜索提示 */
	connect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setPreEnrollMsg(int,int)));
	promptDialog->exec();
	disconnect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
	disconnect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setPreEnrollMsg(int,int)));
}

/**
 * @brief 特征录入 DBus 异步回调函数
 * @param callbackReply
 */
void MainWindow::enrollCallback(QDBusMessage callbackReply)
{
	int result;
	result = callbackReply.arguments()[0].value<int>();
	QList<QStandardItem *> row;
	switch(result) {
	case DBUS_RESULT_SUCCESS: /* 录入成功 */
		row.append(new QStandardItem(indexName));
		row.append(new QStandardItem(QString::number(freeIndex)));
		dataModelMap.value(currentBiotype)->appendRow(row);
		promptDialog->setLabelText(tr("Enroll successfully"));
		break;
	case DBUS_RESULT_ERROR: /* 录入未成功，具体原因还需要进一步读取底层设备的操作状态 */
		{
		biometricIndexMap.value(currentBiotype)->removeOne(freeIndex);
		QDBusPendingReply<int, int, int, int, int> reply =
				biometricInterface->UpdateStatus(
				deviceInfoMap.value(currentBiotype)->driver_id);
		reply.waitForFinished();
		if (reply.isError()) {
			qDebug() << "GUI:" << reply.error();
			promptDialog->setLabelText(tr("D-Bus calling error"));
			return;
		}
		int opsStatus = reply.argumentAt(3).value<int>();
		opsStatus = opsStatus % 100;
		if (opsStatus == OPS_FAILED)
			promptDialog->setLabelText(tr("Failed to enroll"));
		else if (opsStatus == OPS_ERROR)/* 设备底层发生了错误 */
			promptDialog->setLabelText(tr("Device encounters an error"));
		else if (opsStatus == OPS_CANCEL) /* 用户取消 */
			promptDialog->setLabelText(tr("Canceled by user"));
		else if (opsStatus == OPS_TIMEOUT) /* 超时未操作 */
			promptDialog->setLabelText(tr("Operation timeout"));
		break;
		}
	case DBUS_RESULT_DEVICEBUSY: /* 设备忙 */
		biometricIndexMap.value(currentBiotype)->removeOne(freeIndex);
		promptDialog->setLabelText(tr("Device is busy"));
		break;
	case DBUS_RESULT_NOSUCHDEVICE: /* 设备不存在 */
		biometricIndexMap.value(currentBiotype)->removeOne(freeIndex);
		promptDialog->setLabelText(tr("No such device"));
		break;
	case DBUS_RESULT_PERMISSIONDENIED: /* 没有权限 */
		biometricIndexMap.value(currentBiotype)->removeOne(freeIndex);
		promptDialog->setLabelText(tr("Permission denied"));
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
 * @brief 设置操作进度提示，被 D-Bus 信号 StatusChanged 触发
 * @param driverID
 * @param statusType
 */
void MainWindow::setOperationMsg(int driverID, int statusType)
{
	if (statusType != STATUS_NOTIFY)
		return;
	int currentDriverID = deviceInfoMap.value(currentBiotype)->driver_id;
	if (driverID != currentDriverID)
		return;
	QDBusPendingReply<QString> reply =
			biometricInterface->GetNotifyMesg(
				deviceInfoMap.value(currentBiotype)->driver_id);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		promptDialog->setLabelText(tr("Failed to get notify message"));
		return;
	}

	QString msg;
	msg = reply.argumentAt(0).value<QString>();
	promptDialog->setLabelText(msg);
}

/**
 * @brief 针对录入操作，在等待Polkit授权的阶段输出特殊的提示
 * @param driverID
 * @param statusType
 */
void MainWindow::setPreEnrollMsg(int driverID, int statusType)
{
	if (statusType != STATUS_NOTIFY)
		return;
	int currentDriverID = deviceInfoMap.value(currentBiotype)->driver_id;
	if (driverID != currentDriverID)
		return;
	QDBusPendingReply<int, int, int, int, int> reply =
			biometricInterface->UpdateStatus(
				deviceInfoMap.value(currentBiotype)->driver_id);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}

	int devStatus = reply.argumentAt(2).value<int>();
	/*
	 * 如果此时设备正在进行搜索操作(Polkit等待授权)则将弹窗上的搜索提示覆盖，防止用户迷惑
	 * devStatus=101/601/901 分别对应打开设备、正在搜索、关闭设备三种动作
	 */
	if (devStatus ==101 || devStatus == 601 || devStatus == 901)
		promptDialog->setLabelText(tr("Permission is required. Please "
					      "authenticate yourself to continue"));
}

/**
 * @brief 点击取消按钮时被间接触发
 */
void MainWindow::cancelOperation()
{
	QList<QVariant> args;
	args << QVariant(deviceInfoMap.value(currentBiotype)->driver_id)
		<< QVariant(5);
	biometricInterface->callWithCallback("StopOps", args, this,
						SLOT(cancelCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog->setLabelText(tr("In progress, please wait..."));
}

/**
 * @brief 取消成功后的 DBus 回调函数
 * @param reply
 */
void MainWindow::cancelCallback(QDBusMessage callbackReply)
{
	promptDialog->closeDialog();
}

/**
 * @brief 删除生物特征
 */
void MainWindow::on_btnDelete_clicked()
{
	/* 用户光标点击的 QModelIndex */
	QModelIndex clickedModelIndex = treeViewMap.value(currentBiotype)->currentIndex();
	if (clickedModelIndex.row() == -1)
		return;
	/* 获取特征 index 所在的 QModelIndex */
	QModelIndex indexModelIndex = dataModelMap.value(currentBiotype)->index(
					clickedModelIndex.row(), 1,
					clickedModelIndex.parent()
					);
	int deleteIndex = indexModelIndex.data().value<QString>().toInt();

	QDBusPendingReply<int> reply = biometricInterface->Clean(
				deviceInfoMap.value(currentBiotype)->driver_id,
				currentUid, deleteIndex, deleteIndex);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}
	int result = reply.argumentAt(0).value<int>();
	if (result != 0) /* 操作失败，可能是没有权限 */
		return;
	biometricIndexMap.value(currentBiotype)->removeOne(deleteIndex);
	dataModelMap.value(currentBiotype)->removeRow(clickedModelIndex.row(),
						clickedModelIndex.parent());
}

/**
 * @brief 清空当前设备的所有特征值存储
 */
void MainWindow::on_btnDrop_clicked()
{
	QDBusPendingReply<int> reply = biometricInterface->Clean(
				deviceInfoMap.value(currentBiotype)->driver_id,
				currentUid, 0, -1);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}
	biometricIndexMap.value(currentBiotype)->clear();
	dataModelMap.value(currentBiotype)->setRowCount(0);
}

/**
 * @brief 生物特征验证
 */
void MainWindow::on_btnVerify_clicked()
{
	QList<QVariant> args;
	QModelIndex clickedModelIndex, indexModelIndex;
	int verifyIndex;
	clickedModelIndex = treeViewMap.value(currentBiotype)->currentIndex();
	if (clickedModelIndex.row() == -1)
		return;
	indexModelIndex = dataModelMap.value(currentBiotype)->index(
					clickedModelIndex.row(), 1,
					clickedModelIndex.parent()
					);
	verifyIndex = indexModelIndex.data().value<QString>().toInt();
	args << QVariant(deviceInfoMap.value(currentBiotype)->driver_id)
		<< QVariant(currentUid) << QVariant(verifyIndex);
	biometricInterface->callWithCallback("Verify", args, this,
						SLOT(verifyCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog = new PromptDialog(gifMap.value(currentBiotype), this);
	promptDialog->onlyShowCancel();
	connect(promptDialog, &PromptDialog::canceled, this, &MainWindow::cancelOperation);
	connect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
	promptDialog->exec();
	disconnect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
}

/**
 * @brief 特征验证 DBus 异步回调函数
 * @param callbackReply
 */
void MainWindow::verifyCallback(QDBusMessage callbackReply)
{
	promptDialog->onlyShowOK();
}

/**
 * @brief 生物特征搜索
 */
void MainWindow::on_btnSearch_clicked()
{
	QList<QVariant> args;
	args << QVariant(deviceInfoMap.value(currentBiotype)->driver_id)
		<< QVariant(currentUid) << QVariant(0) << QVariant(-1);
	biometricInterface->callWithCallback("Search", args, this,
						SLOT(searchCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog = new PromptDialog(gifMap.value(currentBiotype), this);
	promptDialog->onlyShowCancel();
	connect(promptDialog, &PromptDialog::canceled, this, &MainWindow::cancelOperation);
	connect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
	promptDialog->exec();
	disconnect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
}

/**
 * @brief 特征搜索 DBus 异步回调函数
 * @param callbackReply
 */
void MainWindow::searchCallback(QDBusMessage callbackReply)
{
	QList<QVariant> variantList = callbackReply.arguments();
	int result, hitUid, hitIndex;
	QString hitUname, msg, hitIndexName;
	result = variantList[0].value<int>();
	hitUid = variantList[1].value<int>();
	hitIndex = variantList[2].value<int>();
	hitIndexName = variantList[3].value<QString>();

	/* 经过测试，本函数会在最后一次 StatusChanged 信号触发后才会执行，所以可以将提示信息覆盖一下 */
	/* 没搜到/超时/用户取消 */
	if (result != 0){
		promptDialog->setLabelText(tr("Not Found"));
		promptDialog->onlyShowOK();
		return;
	}

	for (int i = 0; i < ui->comboBoxUname->count(); i++) {
		if (ui->comboBoxUname->itemData(i).value<int>() == hitUid)
			hitUname = ui->comboBoxUname->itemText(i);
	}
	msg = QString(tr("Found! Username: %1, Feature name: %2")).arg(hitUname).arg(hitIndexName);
	promptDialog->setLabelText(msg);
	promptDialog->onlyShowOK();
}
