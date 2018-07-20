#include "contentpane.h"
#include "ui_contentpane.h"
#include <QInputDialog>
#include "promptdialog.h"
#include "inputdialog.h"
#include "messagedialog.h"


#define ICON_SIZE 32


ContentPane::ContentPane(int uid, DeviceInfo *deviceInfo, QWidget *parent) :
	QWidget(parent),
    ui(new Ui::ContentPane),
    deviceInfo(deviceInfo),
    currentUid(uid),
    dataModel(nullptr)
{
    ui->setupUi(this);
	/* 向 QDBus 类型系统注册自定义数据类型 */
	registerCustomTypes();
	/* 连接 DBus Daemon */
    serviceInterface = new QDBusInterface("cn.kylinos.Biometric", "/cn/kylinos/Biometric",
                                          "cn.kylinos.Biometric", QDBusConnection::systemBus());
    serviceInterface->setTimeout(2147483647); /* 微秒 */
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

/**
 * @brief 设置数据模型
 */
void ContentPane::setModel()
{
	/* 设置 TreeView 的 Model */
    dataModel = new TreeModel(currentUid, BioType(deviceInfo->biotype), this);
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
        << QVariant((isAdmin(currentUid) ? -1 : currentUid)) << QVariant(0) << QVariant(-1);
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
    QList<QDBusVariant> qlist;
    FeatureInfo *featureInfo;
	int listsize;
	QList<QVariant> variantList = callbackReply.arguments();
	listsize = variantList[0].value<int>();
	variantList[1].value<QDBusArgument>() >> qlist;
	for (int i = 0; i < listsize; i++) {
        featureInfo = new FeatureInfo;
        qlist[i].variant().value<QDBusArgument>() >> *featureInfo;
        dataModel->appendData(featureInfo);
	}

    QApplication::restoreOverrideCursor();
	updateButtonUsefulness();
}

QString ContentPane::inputFeatureName(bool isNew)
{
    InputDialog *inputDialog = new InputDialog(this);
    if(isNew) {
        inputDialog->setTitle(tr("New Feature"));
        inputDialog->setPrompt(tr("Please input a name for the feature:"));
    } else {
        inputDialog->setTitle(tr("Rename Feature"));
        inputDialog->setPrompt(tr("Please input a new name for the feature:"));
    }
    connect(inputDialog, &InputDialog::dataChanged, this, [&](const QString &text){
        if(dataModel->hasFeature(currentUid, text)) {
            inputDialog->setError(tr("Duplicate feature name"));
        } else if(text.isEmpty()) {
            inputDialog->setError(tr("Empty feature name"));
        } else {
            inputDialog->accept();
        }
    });
    QString featureName = QString();
    if(inputDialog->exec() != QDialog::Rejected)
        featureName = inputDialog->getText();

    delete inputDialog;

    return featureName;
}

/**
 * @brief 录入
 */
void ContentPane::on_btnEnroll_clicked()
{
    indexName = inputFeatureName(true);
    if(indexName.isEmpty())
        return;

    /* 录入的特征索引 */
    freeIndex = dataModel->freeIndex();
    qDebug() << "Enroll: uid--" << currentUid << " index--" << freeIndex
             << " indexName--" << indexName;
    promptDialog = new PromptDialog(serviceInterface, deviceInfo->biotype,
                                    deviceInfo->device_id, currentUid, this);
    promptDialog->enroll(deviceInfo->device_id, currentUid, freeIndex, indexName);
    qDebug() << "Enroll result: ----- " << promptDialog->getResult();
    if(promptDialog->getResult() == PromptDialog::SUCESS) {
        FeatureInfo *featureInfo = createNewFeatureInfo();
        dataModel->appendData(featureInfo);
        updateButtonUsefulness();
    }
    delete promptDialog;
}

FeatureInfo *ContentPane::createNewFeatureInfo()
{
    FeatureInfo *featureInfo = new FeatureInfo;
    featureInfo->uid = currentUid;
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

    QString errMsg;

    for(auto index : selectedIndexList) {
        int idxStart, idxEnd;
        int uid = index.data(TreeModel::UidRole).toInt();
        bool recursive = !ui->treeView->isExpanded(index) &&
                (index.child(0, 0) != QModelIndex());
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

        if (result != DBUS_RESULT_SUCCESS) { /* 操作失败，可能是没有权限 */
            errMsg = getErrorMessage(DELETE, result);
            continue;
        }

        dataModel->removeRow(index.row(), index.parent(), recursive);
    }
    updateButtonUsefulness();

    QString resultMessage;
    int type;
    if(errMsg.isEmpty()) {
        //全部成功删除
        resultMessage = tr("Delete all selected features successfully");
        type = MessageDialog::Normal;
    } else {
        type = MessageDialog::Error;
    }
    MessageDialog msgDialog(type);
    msgDialog.setTitle(tr("Delete Result"));
    msgDialog.setMessage(resultMessage);
    msgDialog.exec();
}

/**
 * @brief 清空当前设备的所有特征值存储
 */
void ContentPane::on_btnClean_clicked()
{
    QDBusPendingReply<int> reply = serviceInterface->call("Clean",
                    deviceInfo->device_id, currentUid, 0, -1);
    reply.waitForFinished();
    if (reply.isError()) {
        qDebug() << "DBUS:" << reply.error();
        return;
    }
    int result = reply.argumentAt(0).value<int>();

    QString resultMessage;
    int type;
    if(result == DBUS_RESULT_SUCCESS) {
        dataModel->removeAll();
        updateButtonUsefulness();
        resultMessage = tr("Clean Successfully");
        type = MessageDialog::Normal;
    } else {
        resultMessage = getErrorMessage(CLEAN, result);
        type = MessageDialog::Error;
    }
    MessageDialog msgDialog(type);
    msgDialog.setTitle(tr("Clean Result"));
    msgDialog.setMessage(resultMessage);
    msgDialog.exec();
}

/**
 * @brief 生物特征验证
 */
void ContentPane::on_btnVerify_clicked()
{
    QModelIndex currentModelIndex;
    int verifyIndex, uid;

    currentModelIndex = ui->treeView->selectionModel()->currentIndex();
    verifyIndex = currentModelIndex.data(Qt::UserRole).toInt();
    uid = currentModelIndex.data(TreeModel::UidRole).toInt();

    promptDialog = new PromptDialog(serviceInterface, deviceInfo->biotype,
                                    deviceInfo->device_id, currentUid, this);
    promptDialog->verify(deviceInfo->device_id, uid, verifyIndex);

    delete promptDialog;
}


/**
 * @brief 生物特征搜索
 */
void ContentPane::on_btnSearch_clicked()
{
    promptDialog = new PromptDialog(serviceInterface, deviceInfo->biotype,
                                    deviceInfo->device_id, currentUid, this);
    promptDialog->search(deviceInfo->device_id, currentUid, 0, -1);

    delete promptDialog;
}

/**
 * @brief 双击重命名
 */
void ContentPane::on_treeView_doubleClicked(const QModelIndex &index)
{
    int column = index.column();

    if(isAdmin(currentUid)) {
        if(column != 2)     //管理员模式双击第三列（特征名称列）重命名
            return;
    } else {
        if(column != 1)     //非管理员模式双击第二列
            return;
    }

    int idx = index.data(TreeModel::IndexRole).toInt();
    int uid = index.data(TreeModel::UidRole).toInt();
    QString idxName = index.data().toString();

    QString newName = inputFeatureName(false);
    if(newName.isEmpty())
        return;

    qDebug() << "Rename " << idx <<idxName << " to " << newName;

    QDBusReply<int> result = serviceInterface->call("Rename", deviceInfo->device_id,
                                                    uid, idx, newName);

    QString resultMessage;
    int type;
    if(result == DBUS_RESULT_SUCCESS) {
        dataModel->setData(index, newName);
        resultMessage = tr("Rename Successfully");
        type = MessageDialog::Normal;
    } else {
        resultMessage = getErrorMessage(RENAME, result);
        type = MessageDialog::Error;
    }
    MessageDialog msgDialog(type);
    msgDialog.setTitle(tr("Rename Result"));
    msgDialog.setMessage(resultMessage);
    msgDialog.exec();
}

QString ContentPane::getErrorMessage(int type, int result)
{
    QString errorMessage;

    switch(result) {
    case DBUS_RESULT_ERROR: {
        //操作失败，需要进一步获取失败原因
        QDBusPendingReply<int, int, int, int, int, int> reply =
                serviceInterface->call("UpdateStatus", deviceInfo->device_id);
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << "DBUS:" << reply.error();
            errorMessage = (tr("D-Bus calling error"));
            break;
        }
        int opsStatus = reply.argumentAt(OPS_STATUS_INDEX).value<int>();
        opsStatus = opsStatus % 100;
        qDebug() << "Ops Error: " << opsStatus;
        switch(opsStatus) {
        case OPS_FAILED:
            switch(type) {
            case DELETE:
                errorMessage = tr("Delete Failed");
                break;
            case CLEAN:
                errorMessage = tr("Clean Failed");
                break;
            case RENAME:
                errorMessage = tr("Rename Failed");
                break;
            }
            break;
        case OPS_ERROR:
            //设备底层发生了错误
            errorMessage = (tr("Device encounters an error"));
            break;
        case OPS_CANCEL:
            //用户取消了操作，直接返回
            break;
        case OPS_TIMEOUT:
            //超时
            errorMessage = (tr("Operation timeout"));
            break;;
        }
        break;
    }
    case DBUS_RESULT_DEVICEBUSY:
        //设备忙
        errorMessage = (tr("Device is busy"));
        break;
    case DBUS_RESULT_NOSUCHDEVICE:
        //设备不存在
        errorMessage = (tr("No such device"));
        break;
    case DBUS_RESULT_PERMISSIONDENIED:
        //没有权限
        errorMessage = (tr("Permission denied"));
        break;
    }
    return errorMessage;
}

