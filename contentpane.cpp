#include <QInputDialog>
#include "promptdialog.h"
#include "contentpane.h"
#include "ui_contentpane.h"

#define ICON_SIZE 32

ContentPane::ContentPane(DeviceInfo *deviceInfo, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ContentPane)
{
	ui->setupUi(this);
	this->deviceInfo = deviceInfo;
	this->selectedUid = -1;
	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
	biometricInterface = new cn::kylinos::Biometric("cn.kylinos.Biometric",
				"/cn/kylinos/Biometric",
				QDBusConnection::systemBus(), this);
	biometricInterface->setTimeout(2147483647); /* 微秒 */
	setPromptDialogGIF();
	setButtonIcons();
	/* 设置数据模型 */
	setModel();
	trackUsedBiometricIndex();
	showDeviceInfo();
	showBiometrics();
}

ContentPane::~ContentPane()
{
	delete ui;
}

void ContentPane::setButtonIcons()
{
	ui->btnEnroll->setIcon(QIcon(":/images/assets/enroll.png"));
	ui->btnEnroll->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	ui->btnDelete->setIcon(QIcon(":/images/assets/delete.png"));
	ui->btnDelete->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	ui->btnVerify->setIcon(QIcon(":/images/assets/verify.png"));
	ui->btnVerify->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	ui->btnSearch->setIcon(QIcon(":/images/assets/search.png"));
	ui->btnSearch->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
	ui->btnDrop->setIcon(QIcon(":/images/assets/drop.png"));
	ui->btnDrop->setIconSize(QSize(ICON_SIZE, ICON_SIZE));
}

void ContentPane::setPromptDialogGIF()
{
	switch (deviceInfo->biotype) {
	case BIOTYPE_FINGERPRINT:
		promptDialogGIF = ":/images/assets/fingerprint.gif";
		break;
	case BIOTYPE_FINGERVEIN:
		promptDialogGIF = ":/images/assets/fingervein.gif";
		break;
	case BIOTYPE_IRIS:
		promptDialogGIF = ":/images/assets/iris.gif";
		break;
	}
}

/**
 * @brief 设置数据模型
 */
void ContentPane::setModel()
{
	/* 设置 TreeView 的 Model */
	dataModel = new QStandardItemModel();
	ui->treeView->setModel(dataModel);
	dataModel->setHorizontalHeaderLabels(
			QStringList() << QString(tr("Feature name")) << QString(tr("index")));
	ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
}

void ContentPane::setSelectedUser(int uid)
{
	selectedUid = uid;
	showBiometrics();
}

void ContentPane::setDriverEnable(bool state)
{
	deviceInfo->driver_enable = state;
}

void ContentPane::setDeviceAvailable(bool state)
{
	deviceInfo->device_available = state;
	if (deviceInfo->device_available)
		ui->labelStatus->setText(tr("Enabled"));
	else
		ui->labelStatus->setText(tr("Disabled"));
	ui->btnEnroll->setEnabled(state);
	ui->btnDelete->setEnabled(state);
	ui->btnVerify->setEnabled(state);
	ui->btnSearch->setEnabled(state);
	ui->btnDrop->setEnabled(state);
	ui->treeView->setEnabled(state);
}

/**
 * @brief 检测某种生物识别设备是否存在
 * @param biotype
 * @return
 */
bool ContentPane::deviceIsAvailable()
{
	return deviceInfo->device_available;
}


void ContentPane::showDeviceInfo()
{
	ui->labelDeviceShortName->setText(deviceInfo->device_shortname);
	setDeviceAvailable(deviceInfo->device_available);
	ui->labelDeviceFullName->setText(deviceInfo->device_fullname);

	QString text;
	if (deviceInfo->biotype == BIOTYPE_FINGERPRINT)
		text = QString(tr("Fingerprint"));
	else if (deviceInfo->biotype == BIOTYPE_FINGERVEIN)
		text = QString(tr("Fingervein"));
	else if (deviceInfo->biotype == BIOTYPE_IRIS)
		text = QString(tr("Iris"));
	ui->labelBiometricType->setText(text);

	if (deviceInfo->vertype == VERIFY_HARDWARE)
		text = QString(tr("Hardware Verification"));
	else if (deviceInfo->vertype == VERIFY_SOFTWARE)
		text = QString(tr("Software Verification"));
	else if (deviceInfo->vertype == VERIFY_MIX)
		text = QString(tr("Mix Verification"));
	else if (deviceInfo->vertype == VERIFY_OTHER)
		text = QString(tr("Other Verification"));
	ui->labelVerifyType->setText(text);

	if (deviceInfo->bustype == BUS_SERIAL)
		text = QString(tr("Serial"));
	else if (deviceInfo->bustype == BUS_USB)
		text = QString(tr("USB"));
	else if (deviceInfo->bustype == BUS_PCIE)
		text = QString(tr("PCIE"));
	ui->labelBusType->setText(text);

	if (deviceInfo->stotype == STORAGE_DEVICE)
		text = QString(tr("Device Storage"));
	else if (deviceInfo->stotype == STORAGE_OS)
		text = QString(tr("OS Storage"));
	else if (deviceInfo->stotype == STORAGE_MIX)
		text = QString(tr("Mix Storage"));
	ui->labelStorageType->setText(text);

	if (deviceInfo->idtype == IDENTIFY_HARDWARE)
		text = QString(tr("Hardware Identification"));
	else if (deviceInfo->idtype == IDENTIFY_SOFTWARE)
		text = QString(tr("Software Identification"));
	else if (deviceInfo->idtype == IDENTIFY_MIX)
		text = QString(tr("Mix Identification"));
	else if (deviceInfo->idtype == IDENTIFY_OTHER)
		text = QString(tr("Other Identification"));
	ui->labelIdentificationType->setText(text);
}

/**
 * @brief 获取所有设备已经被使用的生物特征 index 列表
 */
void ContentPane::trackUsedBiometricIndex()
{
	if (!deviceIsAvailable())
		return;

	QList<QDBusVariant> qlist;
	BiometricInfo *biometricInfo;
	int listsize;

	QDBusPendingReply<int, QList<QDBusVariant> > reply =
		biometricInterface->GetFeatureList(deviceInfo->device_id, -1, 0, -1);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}

	listsize = reply.argumentAt(0).value<int>();
	reply.argumentAt(1).value<QDBusArgument>() >> qlist;

	usedIndexList = new QList<int>;
	for (int j = 0; j < listsize; j++) {
		biometricInfo = new BiometricInfo();
		qlist[j].variant().value<QDBusArgument>() >> *biometricInfo;
		usedIndexList->append(biometricInfo->index);
	}
	qSort(*usedIndexList);
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
int ContentPane::findFreeBiometricIndex()
{
	int freeIndex, boundaryPos;
	/* 插入占位index,值为0,简化处理逻辑的同时也保证index从1开始利用，不会在前面出现断层 */
	usedIndexList->prepend(0);
	/* boundaryPos 是空隙的左边界下标 */
	boundaryPos = binary_search(*usedIndexList, 0, usedIndexList->length()-1);
	if (boundaryPos == -1) /* 整个列表都是连续的 */
		boundaryPos = usedIndexList->length() -1;
	freeIndex = usedIndexList->at(boundaryPos) + 1;
	/* 将空闲index插入追踪列表 */
	usedIndexList->insert(boundaryPos + 1, freeIndex);
	/* 删除插入的占位元素 */
	usedIndexList->removeFirst();

	return freeIndex;
}

/**
 * @brief 显示生物识别数据列表
 * @param biotype
 */
void ContentPane::showBiometrics()
{
	if (!deviceIsAvailable())
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	QList<QVariant> args;

	/* 不能用clear()，它会将表头也清掉 */
	dataModel->setRowCount(0);

	args << QVariant(deviceInfo->device_id)
		<< QVariant(selectedUid) << QVariant(0) << QVariant(-1);
	biometricInterface->callWithCallback("GetFeatureList", args, this,
						SLOT(showBiometricsCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
}

/**
 * @brief 特征列表返回后回调函数进行显示
 * @param callbackReply
 */
void ContentPane::showBiometricsCallback(QDBusMessage callbackReply)
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
		dataModel->appendRow(row);
	}
	QApplication::restoreOverrideCursor();
}

/**
 * @brief 录入
 */
void ContentPane::on_btnEnroll_clicked()
{
	QList<QVariant> args;
	bool ok;
	QInputDialog *inputDialog = new QInputDialog();
	inputDialog->setOkButtonText(tr("OK"));
	inputDialog->setCancelButtonText(tr("Cancel"));
	QString text = inputDialog->getText(this, tr("Please input a feature name"),
					tr("Feature name"), QLineEdit::Normal,
					"", &ok);
	if (!ok || text.isEmpty())
		return;
	indexName = text;
	/* 查找空闲 index */
	freeIndex = findFreeBiometricIndex();
	/*
	 * 异步回调参考资料：
	 * https://github.com/RalfVB/SQEW-OS/blob/master/src/module/windowmanager/compton.cpp
	 */
	args << QVariant(deviceInfo->device_id)
		<< QVariant(selectedUid) << QVariant(freeIndex) << QVariant(indexName);
	biometricInterface->callWithCallback("Enroll", args, this,
						SLOT(enrollCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog = new PromptDialog(promptDialogGIF, this);
	promptDialog->onlyShowCancel();
	/*
	connect(promptDialog, &PromptDialog::rejected, [this]{
						this->canceledByUser=true;
						});*/
	connect(promptDialog, &PromptDialog::canceled, this, &ContentPane::cancelOperation);
	connect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
	/* 绑定第二个 SLOT，判别录入过程等待 Polkit 授权的阶段，输出特殊提示覆盖 Polkit 搜索操作造成的搜索提示 */
	connect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setPreEnrollMsg(int,int)));
	promptDialog->exec();
}

/**
 * @brief 特征录入 DBus 异步回调函数
 * @param callbackReply
 */
void ContentPane::enrollCallback(QDBusMessage callbackReply)
{
	/* 最终结果信息由本函数输出，断开操作信息的信号触发 */
	disconnect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
	disconnect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setPreEnrollMsg(int,int)));

	int result;
	result = callbackReply.arguments()[0].value<int>();
	QList<QStandardItem *> row;
	switch(result) {
	case DBUS_RESULT_SUCCESS: /* 录入成功 */
		row.append(new QStandardItem(indexName));
		row.append(new QStandardItem(QString::number(freeIndex)));
		dataModel->appendRow(row);
		promptDialog->setLabelText(tr("Enroll successfully"));
		break;
	case DBUS_RESULT_ERROR: /* 录入未成功，具体原因还需要进一步读取底层设备的操作状态 */
		{
		usedIndexList->removeOne(freeIndex);
		QDBusPendingReply<int, int, int, int, int> reply =
				biometricInterface->UpdateStatus(deviceInfo->device_id);
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
			return; /* 对于取消操作，弹窗会迅速关闭所以不需要设置信息或改变按钮 */
		else if (opsStatus == OPS_TIMEOUT) /* 超时未操作 */
			promptDialog->setLabelText(tr("Operation timeout"));
		break;
		}
	case DBUS_RESULT_DEVICEBUSY: /* 设备忙 */
		usedIndexList->removeOne(freeIndex);
		promptDialog->setLabelText(tr("Device is busy"));
		break;
	case DBUS_RESULT_NOSUCHDEVICE: /* 设备不存在 */
		usedIndexList->removeOne(freeIndex);
		promptDialog->setLabelText(tr("No such device"));
		break;
	case DBUS_RESULT_PERMISSIONDENIED: /* 没有权限 */
		usedIndexList->removeOne(freeIndex);
		promptDialog->setLabelText(tr("Permission denied"));
		break;
	}
	promptDialog->onlyShowOK();
}

/**
 * @brief DBus 异步操作 errorMethod
 * @param error
 */
void ContentPane::errorCallback(QDBusError error)
{
	qDebug() << "GUI:" << error.message();
}

/**
 * @brief 设置操作进度提示，被 D-Bus 信号 StatusChanged 触发
 * @param deviceID
 * @param statusType
 */
void ContentPane::setOperationMsg(int deviceID, int statusType)
{
	if (!(deviceID == deviceInfo->device_id && statusType == STATUS_NOTIFY))
		return;
	QDBusPendingReply<QString> reply =
			biometricInterface->GetNotifyMesg(deviceInfo->device_id);
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
 * @param deviceID
 * @param statusType
 */
void ContentPane::setPreEnrollMsg(int deviceID, int statusType)
{
	if (!(deviceID == deviceInfo->device_id && statusType == STATUS_NOTIFY))
		return;
	QDBusPendingReply<int, int, int, int, int> reply =
			biometricInterface->UpdateStatus(deviceInfo->device_id);
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
void ContentPane::cancelOperation()
{
	QList<QVariant> args;
	args << QVariant(deviceInfo->device_id) << QVariant(5);
	biometricInterface->callWithCallback("StopOps", args, this,
						SLOT(cancelCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog->setLabelText(tr("In progress, please wait..."));
}

/**
 * @brief 取消成功后的 DBus 回调函数
 * @param reply
 */
void ContentPane::cancelCallback(QDBusMessage callbackReply)
{
	UNUSED(callbackReply);
	promptDialog->closeDialog();
}

/**
 * @brief 删除生物特征
 */
void ContentPane::on_btnDelete_clicked()
{
	/* 用户光标点击的 QModelIndex */
	QModelIndex clickedModelIndex = ui->treeView->currentIndex();
	if (clickedModelIndex.row() == -1)
		return;
	/* 获取特征 index 所在的 QModelIndex */
	QModelIndex indexModelIndex = dataModel->index(clickedModelIndex.row(),
						1, clickedModelIndex.parent());
	int deleteIndex = indexModelIndex.data().value<QString>().toInt();

	QDBusPendingReply<int> reply = biometricInterface->Clean(
			deviceInfo->device_id, selectedUid, deleteIndex, deleteIndex);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}
	int result = reply.argumentAt(0).value<int>();
	if (result != DBUS_RESULT_SUCCESS) /* 操作失败，可能是没有权限 */
		return;
	usedIndexList->removeOne(deleteIndex);
	dataModel->removeRow(clickedModelIndex.row(), clickedModelIndex.parent());
}

/**
 * @brief 清空当前设备的所有特征值存储
 */
void ContentPane::on_btnDrop_clicked()
{
	QDBusPendingReply<int> reply = biometricInterface->Clean(
					deviceInfo->device_id, selectedUid, 0, -1);
	reply.waitForFinished();
	if (reply.isError()) {
		qDebug() << "GUI:" << reply.error();
		return;
	}
	int result = reply.argumentAt(0).value<int>();
	if (result != DBUS_RESULT_SUCCESS) /* 操作失败，可能是没有权限 */
		return;
	usedIndexList->clear();
	dataModel->setRowCount(0);
}

/**
 * @brief 生物特征验证
 */
void ContentPane::on_btnVerify_clicked()
{
	QList<QVariant> args;
	QModelIndex clickedModelIndex, indexModelIndex;
	int verifyIndex;
	clickedModelIndex = ui->treeView->currentIndex();
	if (clickedModelIndex.row() == -1)
		return;
	indexModelIndex = dataModel->index(clickedModelIndex.row(), 1,
						clickedModelIndex.parent());
	verifyIndex = indexModelIndex.data().value<QString>().toInt();
	args << QVariant(deviceInfo->device_id) << QVariant(selectedUid)
						<< QVariant(verifyIndex);
	biometricInterface->callWithCallback("Verify", args, this,
						SLOT(verifyCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog = new PromptDialog(promptDialogGIF, this);
	promptDialog->onlyShowCancel();
	connect(promptDialog, &PromptDialog::canceled, this, &ContentPane::cancelOperation);
	connect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
	promptDialog->exec();
}

/**
 * @brief 特征验证 DBus 异步回调函数
 * @param callbackReply
 */
void ContentPane::verifyCallback(QDBusMessage callbackReply)
{
	disconnect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));

	int result;
	result = callbackReply.arguments()[0].value<int>();
	QList<QStandardItem *> row;
	switch(result) {
	case DBUS_RESULT_SUCCESS:
		promptDialog->setLabelText(tr("Match successfully"));
		break;
	case DBUS_RESULT_ERROR:
		{
		QDBusPendingReply<int, int, int, int, int> reply =
				biometricInterface->UpdateStatus(deviceInfo->device_id);
		reply.waitForFinished();
		if (reply.isError()) {
			qDebug() << "GUI:" << reply.error();
			promptDialog->setLabelText(tr("D-Bus calling error"));
			return;
		}
		int opsStatus = reply.argumentAt(3).value<int>();
		opsStatus = opsStatus % 100;
		if (opsStatus == OPS_FAILED)
			promptDialog->setLabelText(tr("Failed to match"));
		else if (opsStatus == OPS_ERROR)
			promptDialog->setLabelText(tr("Device encounters an error"));
		else if (opsStatus == OPS_CANCEL)
			return;
		else if (opsStatus == OPS_TIMEOUT)
			promptDialog->setLabelText(tr("Operation timeout"));
		break;
		}
	case DBUS_RESULT_DEVICEBUSY:
		promptDialog->setLabelText(tr("Device is busy"));
		break;
	case DBUS_RESULT_NOSUCHDEVICE:
		promptDialog->setLabelText(tr("No such device"));
		break;
	case DBUS_RESULT_PERMISSIONDENIED:
		promptDialog->setLabelText(tr("Permission denied"));
		break;
	}
	promptDialog->onlyShowOK();
}

/**
 * @brief 生物特征搜索
 */
void ContentPane::on_btnSearch_clicked()
{
	QList<QVariant> args;
	args << QVariant(deviceInfo->device_id) << QVariant(selectedUid)
						<< QVariant(0) << QVariant(-1);
	biometricInterface->callWithCallback("Search", args, this,
						SLOT(searchCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog = new PromptDialog(promptDialogGIF, this);
	promptDialog->onlyShowCancel();
	connect(promptDialog, &PromptDialog::canceled, this, &ContentPane::cancelOperation);
	connect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
	promptDialog->exec();
}

/**
 * @brief 特征搜索 DBus 异步回调函数
 * @param callbackReply
 */
void ContentPane::searchCallback(QDBusMessage callbackReply)
{
	disconnect(biometricInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));

	QList<QVariant> variantList = callbackReply.arguments();
	int result, hitUid, hitIndex;
	QString hitUsername, msg, hitIndexName;
	result = variantList[0].value<int>();
	hitUid = variantList[1].value<int>();
	hitIndex = variantList[2].value<int>();
	hitIndexName = variantList[3].value<QString>();
	UNUSED(hitUid);
	UNUSED(hitIndex);

	QList<QStandardItem *> row;
	switch(result) {
	case DBUS_RESULT_SUCCESS:
		hitUsername = "Wait for getting";
		msg = QString(tr("Found! Username: %1, Feature name: %2")).arg(hitUsername).arg(hitIndexName);
		promptDialog->setLabelText(msg);
		break;
	case DBUS_RESULT_ERROR:
		{
		QDBusPendingReply<int, int, int, int, int> reply =
				biometricInterface->UpdateStatus(deviceInfo->device_id);
		reply.waitForFinished();
		if (reply.isError()) {
			qDebug() << "GUI:" << reply.error();
			promptDialog->setLabelText(tr("D-Bus calling error"));
			return;
		}
		int opsStatus = reply.argumentAt(3).value<int>();
		opsStatus = opsStatus % 100;
		if (opsStatus == OPS_FAILED)
			promptDialog->setLabelText(tr("Not Found"));
		else if (opsStatus == OPS_ERROR)
			promptDialog->setLabelText(tr("Device encounters an error"));
		else if (opsStatus == OPS_CANCEL)
			return;
		else if (opsStatus == OPS_TIMEOUT)
			promptDialog->setLabelText(tr("Operation timeout"));
		break;
		}
	case DBUS_RESULT_DEVICEBUSY:
		promptDialog->setLabelText(tr("Device is busy"));
		break;
	case DBUS_RESULT_NOSUCHDEVICE:
		promptDialog->setLabelText(tr("No such device"));
		break;
	case DBUS_RESULT_PERMISSIONDENIED:
		promptDialog->setLabelText(tr("Permission denied"));
		break;
	}
	promptDialog->onlyShowOK();
}
