#include "contentpane.h"
#include "ui_contentpane.h"
#include <QInputDialog>
#include "promptdialog.h"


#define ICON_SIZE 32

#define SET_PROMPT(str) do { \
    if(promptDialog)    \
        promptDialog->setLabelText(str); \
} while(0)


ContentPane::ContentPane(int uid, DeviceInfo *deviceInfo, QWidget *parent) :
	QWidget(parent),
    ui(new Ui::ContentPane),
    deviceInfo(deviceInfo),
    selectedUid(uid),
    dataModel(nullptr),
    isEnrolling(false)
{
    ui->setupUi(this);
	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
    serviceInterface = new QDBusInterface("cn.kylinos.Biometric", "/cn/kylinos/Biometric",
                                          "cn.kylinos.Biometric", QDBusConnection::systemBus());
    serviceInterface->setTimeout(2147483647); /* 微秒 */
	setPromptDialogGIF();
	updateWidgetStatus();
	/* 设置数据模型 */
	setModel();
	showDeviceInfo();
    showFeatures();
}

ContentPane::~ContentPane()
{
	delete ui;
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
    case BIOTYPE_VOICEPRINT:
        promptDialogGIF = ":/images/assets/voiceprint.gif";
        break;
	}
}

/**
 * @brief 设置数据模型
 */
void ContentPane::setModel()
{
	/* 设置 TreeView 的 Model */
    dataModel = new TreeModel(selectedUid, BioType(deviceInfo->biotype), this);
    ui->treeView->setModel(dataModel);
	ui->treeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->treeView->setFocusPolicy(Qt::NoFocus);
}

void ContentPane::setDeviceAvailable(int deviceAvailable)
{
    deviceInfo->device_available = deviceAvailable;
	updateWidgetStatus();
}

int ContentPane::featuresCount()
{
    if(dataModel)
        return dataModel->rowCount();
    return 0;
}

void ContentPane::updateWidgetStatus()
{
    if (deviceInfo->device_available > 0) {
        ui->btnStatus->setStyleSheet("QPushButton{background:url(:/images/assets/switch_open_small.png);}");
        ui->labelStatusText->setText(tr("Opened"));
	} else {
        ui->btnStatus->setStyleSheet("QPushButton{background:url(:/images/assets/switch_close_small.png);}");
        ui->labelStatusText->setText(tr("Closed"));
	}
	ui->btnEnroll->setEnabled(deviceInfo->device_available);
	ui->btnDelete->setEnabled(deviceInfo->device_available);
	ui->btnVerify->setEnabled(deviceInfo->device_available);
	ui->btnSearch->setEnabled(deviceInfo->device_available);
    ui->btnClean->setEnabled(deviceInfo->device_available);
	ui->treeView->setEnabled(deviceInfo->device_available);
}

void ContentPane::updateButtonUsefulness()
{
	bool enable = !!dataModel->rowCount();
	ui->btnDelete->setEnabled(enable);
	ui->btnVerify->setEnabled(enable);
	ui->btnSearch->setEnabled(enable);
    ui->btnClean->setEnabled(enable);
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
    QString verifyType = EnumToString::transferVerifyType(deviceInfo->vertype);
    QString busType = EnumToString::transferBusType(deviceInfo->bustype);
    QString storageType = EnumToString::transferStorageType(deviceInfo->stotype);
    QString identifyType = EnumToString::transferIdentifyType(deviceInfo->idtype);
    QString listName = EnumToString::transferBioType(deviceInfo->biotype) + tr("List");

	ui->labelDeviceShortName->setText(deviceInfo->device_shortname);
	ui->labelDeviceFullName->setText(deviceInfo->device_fullname);
    ui->labelVerifyType->setText(verifyType);
    ui->labelBusType->setText(busType);
    ui->labelStorageType->setText(storageType);
    ui->labelIdentificationType->setText(identifyType);
    ui->labelListName->setText(listName);

    ui->labelDefault->hide();
    ui->btnDefault->hide();
//    ui->btnDefault->setStyleSheet("QPushButton{background:url(:/images/assets/switch_close_small.png);}");
}

void ContentPane::on_btnStatus_clicked()
{
    Q_EMIT changeDeviceStatus(deviceInfo);
}

void ContentPane::on_btnDefault_clicked()
{
}


/**
 * @brief 显示生物识别数据列表
 * @param biotype
 */
void ContentPane::showFeatures()
{
	if (!deviceIsAvailable())
		return;

	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	QList<QVariant> args;

	args << QVariant(deviceInfo->device_id)
        << QVariant((selectedUid == ADMIN_UID ? -1 : selectedUid)) << QVariant(0) << QVariant(-1);
    serviceInterface->callWithCallback("GetFeatureList", args, this,
                        SLOT(showFeaturesCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
}

/**
 * @brief 特征列表返回后回调函数进行显示
 * @param callbackReply
 */
void ContentPane::showFeaturesCallback(QDBusMessage callbackReply)
{
    QList<FeatureInfo*> featureInfoList;

	QList<QDBusVariant> qlist;
    FeatureInfo *featureInfo;
	int listsize;
	QList<QVariant> variantList = callbackReply.arguments();
	listsize = variantList[0].value<int>();
	variantList[1].value<QDBusArgument>() >> qlist;
	for (int i = 0; i < listsize; i++) {
        featureInfo = new FeatureInfo;
        qlist[i].variant().value<QDBusArgument>() >> *featureInfo;
        featureInfoList.append(featureInfo);
	}
    dataModel->setModelData(featureInfoList);

    QApplication::restoreOverrideCursor();
	updateButtonUsefulness();

    //清空列表
    for(auto info : featureInfoList)
        delete info;
    featureInfoList.clear();
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
    QString text = inputDialog->getText(this, tr("Feature name"),
                    tr("Please input a feature name"), QLineEdit::Normal,
                    "", &ok);
    if (!ok || text.isEmpty())
        return;
    indexName = text;
    /* 录入的特征索引 */
    freeIndex = dataModel->freeIndex();
    qDebug() << "Enroll: uid--" << selectedUid << " index--" << freeIndex
             << " indexName--" << indexName;
    /*
     * 异步回调参考资料：
     * https://github.com/RalfVB/SQEW-OS/blob/master/src/module/windowmanager/compton.cpp
     */
    args << QVariant(deviceInfo->device_id)
        << QVariant(selectedUid) << QVariant(freeIndex) << QVariant(indexName);
    serviceInterface->callWithCallback("Enroll", args, this,
                        SLOT(enrollCallback(QDBusMessage)),
                        SLOT(errorCallback(QDBusError)));
    promptDialog = new PromptDialog(promptDialogGIF, nullptr,
                    tr("Permission is required. Please "
                    "authenticate yourself to continue"));
    qDebug() << "Enroll Device: " << deviceInfo->device_id << deviceInfo->device_shortname;
    QString title = EnumToString::transferBioType(deviceInfo->biotype) + tr("Enroll");
    promptDialog->setTitle(title);

    connect(promptDialog, &PromptDialog::canceled, this, &ContentPane::cancelOperation);
    connect(serviceInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));

    isEnrolling = true;
    promptDialog->exec();
}

/**
 * @brief 特征录入 DBus 异步回调函数
 * @param callbackReply
 */
void ContentPane::enrollCallback(QDBusMessage callbackReply)
{
    /* 最终结果信息由本函数输出，断开操作信息的信号触发 */
    disconnect(serviceInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));

    int result;
    result = callbackReply.arguments()[0].value<int>();
    qDebug() << "Enroll result: " << result;

    switch(result) {
    case DBUS_RESULT_SUCCESS: { /* 录入成功 */
        FeatureInfo *featureInfo = createNewFeatureInfo();
        dataModel->appendData(featureInfo);
        updateButtonUsefulness();
        SET_PROMPT(tr("Enroll successfully"));
        break;
    }
    case DBUS_RESULT_ERROR: { /* 录入未成功，具体原因还需要进一步读取底层设备的操作状态 */
        QDBusPendingReply<int, int, int, int, int, int> reply =
                serviceInterface->call("UpdateStatus", deviceInfo->device_id);
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << "DBUS:" << reply.error();
            SET_PROMPT(tr("D-Bus calling error"));
            return;
        }
        int opsStatus = reply.argumentAt(OPS_STATUS_INDEX).value<int>();
        opsStatus = opsStatus % 100;
        if (opsStatus == OPS_FAILED)
            SET_PROMPT(tr("Failed to enroll"));
        else if (opsStatus == OPS_ERROR)/* 设备底层发生了错误 */
            SET_PROMPT(tr("Device encounters an error"));
        else if (opsStatus == OPS_CANCEL) /* 用户取消 */
            return; /* 对于取消操作，弹窗会迅速关闭所以不需要设置信息或改变按钮 */
        else if (opsStatus == OPS_TIMEOUT) /* 超时未操作 */
            SET_PROMPT(tr("Operation timeout"));
        break;
    }
    case DBUS_RESULT_DEVICEBUSY: /* 设备忙 */
        SET_PROMPT(tr("Device is busy"));
        break;
    case DBUS_RESULT_NOSUCHDEVICE: /* 设备不存在 */
        SET_PROMPT(tr("No such device"));
        break;
    case DBUS_RESULT_PERMISSIONDENIED: /* 没有权限 */
        SET_PROMPT(tr("Permission denied"));
        break;
    }
    isEnrolling = false;
}

FeatureInfo *ContentPane::createNewFeatureInfo()
{
    FeatureInfo *featureInfo = new FeatureInfo;
    featureInfo->uid = selectedUid;
    featureInfo->biotype = deviceInfo->biotype;
    featureInfo->device_shortname = deviceInfo->device_shortname;
    featureInfo->index = freeIndex;
    featureInfo->index_name = indexName;
    return featureInfo;
}

/**
 * @brief DBus 异步操作 errorMethod
 * @param error
 */
void ContentPane::errorCallback(QDBusError error)
{
    qDebug() << "DBUS:" << error.message();
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

    //过滤掉当录入时使用生物识别授权接收到的认证的提示信息
    if(isEnrolling) {
        qDebug() << "Enrolling";
        QDBusMessage reply = serviceInterface->call("UpdateStatus", deviceInfo->device_id);
        if(reply.type() == QDBusMessage::ErrorMessage) {
            qDebug() << "DBUS: " << reply.errorMessage();
            return;
        }
        int devStatus = reply.arguments().at(3).toInt();

        if(!(devStatus >= 201 && devStatus < 203))
            return;
    }

    QDBusMessage notifyReply = serviceInterface->call("GetNotifyMesg", deviceInfo->device_id);
    if(notifyReply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "DBUS: " << notifyReply.errorMessage();
        return;
    }
    QString prompt = notifyReply.arguments().at(0).toString();
    qDebug() << prompt;

    SET_PROMPT(prompt);
}


/**
 * @brief 点击取消按钮时被间接触发
 */
void ContentPane::cancelOperation()
{
	QList<QVariant> args;
	args << QVariant(deviceInfo->device_id) << QVariant(5);
    serviceInterface->callWithCallback("StopOps", args, this,
						SLOT(cancelCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
    SET_PROMPT(tr("In progress, please wait..."));
}

/**
 * @brief 取消成功后的 DBus 回调函数
 * @param reply
 */
void ContentPane::cancelCallback(QDBusMessage callbackReply)
{
    UNUSED(callbackReply);
    promptDialog->closeDialog();
    delete promptDialog;
    promptDialog = nullptr;
}

/**
 * @brief 删除生物特征
 */
void ContentPane::on_btnDelete_clicked()
{
    QModelIndexList selectedIndexList = ui->treeView->selectionModel()->selectedRows(0);

    auto bound = std::stable_partition(selectedIndexList.begin(), selectedIndexList.end(),
                          [&](const QModelIndex &index){return index.parent() != QModelIndex();});
    auto cmp = [&](const QModelIndex &ia, const QModelIndex &ib){ return ia.row() > ib.row();};
    std::sort(selectedIndexList.begin(), bound, cmp);
    std::sort(bound, selectedIndexList.end(), cmp);

    for(auto index : selectedIndexList) {
        int idxStart, idxEnd;
        int uid = index.data(TreeModel::UidRole).toInt();
        bool recursive = !ui->treeView->isExpanded(index) && (index.parent() == QModelIndex());
        if(recursive) {
            //如果没有展开则删除该用户的所有特征
            idxStart = 0;
            idxEnd = -1;
        } else {
            idxStart = idxEnd = index.data(Qt::UserRole).toInt();
        }
        qDebug() << "Delete: uid--" << uid << " index--" << idxStart << idxEnd;

        QDBusPendingReply<int> reply = serviceInterface->call("Clean",
                deviceInfo->device_id, uid, idxStart, idxEnd);
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << "DBUS:" << reply.error();
            return;
        }
        int result = reply.argumentAt(0).value<int>();
        qDebug() << "delete result: " << result;

        if (result != DBUS_RESULT_SUCCESS) /* 操作失败，可能是没有权限 */
            continue;

        dataModel->removeRow(index.row(), index.parent(), recursive);
    }

    updateButtonUsefulness();
}

/**
 * @brief 清空当前设备的所有特征值存储
 */
void ContentPane::on_btnClean_clicked()
{
    QDBusPendingReply<int> reply = serviceInterface->call("Clean",
                    deviceInfo->device_id, selectedUid, 0, -1);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "DBUS:" << reply.error();
        return;
    }
    int result = reply.argumentAt(0).value<int>();
    if (result != DBUS_RESULT_SUCCESS) /* 操作失败，可能是没有权限 */
        return;

    dataModel->removeAll();
    updateButtonUsefulness();
}

/**
 * @brief 生物特征验证
 */
void ContentPane::on_btnVerify_clicked()
{
	QList<QVariant> args;
    QModelIndex currentModelIndex;
    int verifyIndex, uid;

    currentModelIndex = ui->treeView->selectionModel()->currentIndex();
    verifyIndex = currentModelIndex.data(Qt::UserRole).toInt();
    uid = currentModelIndex.data(TreeModel::UidRole).toInt();

    promptDialog = new PromptDialog(promptDialogGIF, this);
    QString title = EnumToString::transferBioType(deviceInfo->biotype) + tr("Verify");
    promptDialog->setTitle(title);
    connect(promptDialog, &PromptDialog::canceled, this, &ContentPane::cancelOperation);

    qDebug() << "verify--- uid: " << uid << " index: " << verifyIndex;

    args << QVariant(deviceInfo->device_id) << QVariant(uid)
                        << QVariant(verifyIndex);
    serviceInterface->callWithCallback("Verify", args, this,
                        SLOT(verifyCallback(QDBusMessage)),
                        SLOT(errorCallback(QDBusError)));

    connect(serviceInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
    promptDialog->exec();
    qDebug() << "---";
}

/**
 * @brief 特征验证 DBus 异步回调函数
 * @param callbackReply
 */
void ContentPane::verifyCallback(QDBusMessage callbackReply)
{
    disconnect(serviceInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));

	int result;
	result = callbackReply.arguments()[0].value<int>();
    qDebug() << "Verify result: " << result;

    if(result >= 0)
        SET_PROMPT(tr("Match successfully"));
    else {
        switch(result) {
        case DBUS_RESULT_NOTMATCH:
            SET_PROMPT(tr("Not Match"));
            break;
        case DBUS_RESULT_ERROR:
            {
            QDBusPendingReply<int, int, int, int, int, int> reply =
                    serviceInterface->call("UpdateStatus", deviceInfo->device_id);
            reply.waitForFinished();
            if (reply.isError()) {
                qDebug() << "DBUS:" << reply.error();
                SET_PROMPT(tr("D-Bus calling error"));
                return;
            }
            int opsStatus = reply.argumentAt(OPS_STATUS_INDEX).value<int>();
            opsStatus = opsStatus % 100;
            if (opsStatus == OPS_FAILED)
                SET_PROMPT(tr("Failed to match"));
            else if (opsStatus == OPS_ERROR)
                SET_PROMPT(tr("Device encounters an error"));
            else if (opsStatus == OPS_CANCEL)
                return;
            else if (opsStatus == OPS_TIMEOUT)
                SET_PROMPT(tr("Operation timeout"));
            break;
            }
        case DBUS_RESULT_DEVICEBUSY:
            SET_PROMPT(tr("Device is busy"));
            break;
        case DBUS_RESULT_NOSUCHDEVICE:
            SET_PROMPT(tr("No such device"));
            break;
        case DBUS_RESULT_PERMISSIONDENIED:
            SET_PROMPT(tr("Permission denied"));
            break;
        }
    }
}

/**
 * @brief 生物特征搜索
 */
void ContentPane::on_btnSearch_clicked()
{
	QList<QVariant> args;
	args << QVariant(deviceInfo->device_id) << QVariant(selectedUid)
						<< QVariant(0) << QVariant(-1);
    serviceInterface->callWithCallback("Search", args, this,
						SLOT(searchCallback(QDBusMessage)),
						SLOT(errorCallback(QDBusError)));
	promptDialog = new PromptDialog(promptDialogGIF, this);
    QString title = EnumToString::transferBioType(deviceInfo->biotype) + tr("Search");
    promptDialog->setTitle(title);

	connect(promptDialog, &PromptDialog::canceled, this, &ContentPane::cancelOperation);
    connect(serviceInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));
    promptDialog->exec();
}

/**
 * @brief 特征搜索 DBus 异步回调函数
 * @param callbackReply
 */
void ContentPane::searchCallback(QDBusMessage callbackReply)
{
    disconnect(serviceInterface, SIGNAL(StatusChanged(int,int)), this, SLOT(setOperationMsg(int,int)));

    int count = callbackReply.arguments().at(0).toInt();
    qDebug() << "search result: " << count;

    if(count > 0) {
        SET_PROMPT(tr("Search Result"));
        QDBusArgument argument = callbackReply.arguments().at(1).value<QDBusArgument>();
        QList<QVariant> variantList;
        argument >> variantList;
        QList<SearchResult> results;
        for(int i = 0; i < count; i++) {
            QDBusArgument arg =variantList.at(i).value<QDBusArgument>();
            SearchResult ret;
            arg >> ret;
            results.append(ret);
        }
        promptDialog->setSearchResult(selectedUid == ADMIN_UID, results);
    } else {
        switch(count) {
        case DBUS_RESULT_SUCCESS:
        case DBUS_RESULT_NOTMATCH:
            SET_PROMPT(tr("No matching features Found"));
            break;
        case DBUS_RESULT_ERROR:
            {
            QDBusPendingReply<int, int, int, int, int, int> reply =
                    serviceInterface->call("UpdateStatus", deviceInfo->device_id);
            reply.waitForFinished();
            if (reply.isError()) {
                qDebug() << "GUI:" << reply.error();
                SET_PROMPT(tr("D-Bus calling error"));
                return;
            }
            int opsStatus = reply.argumentAt(OPS_STATUS_INDEX).value<int>();
            opsStatus = opsStatus % 100;
            if (opsStatus == OPS_FAILED)
                SET_PROMPT(tr("Not Found"));
            else if (opsStatus == OPS_ERROR)
                SET_PROMPT(tr("Device encounters an error"));
            else if (opsStatus == OPS_CANCEL)
                return;
            else if (opsStatus == OPS_TIMEOUT)
                SET_PROMPT(tr("Operation timeout"));
            break;
            }
        case DBUS_RESULT_DEVICEBUSY:
            SET_PROMPT(tr("Device is busy"));
            break;
        case DBUS_RESULT_NOSUCHDEVICE:
            SET_PROMPT(tr("No such device"));
            break;
        case DBUS_RESULT_PERMISSIONDENIED:
            SET_PROMPT(tr("Permission denied"));
            break;
        }
    }
}
