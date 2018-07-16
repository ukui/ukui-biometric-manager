#include "contentpane.h"
#include "ui_contentpane.h"
#include <QInputDialog>
#include "promptdialog.h"


#define ICON_SIZE 32


ContentPane::ContentPane(int uid, DeviceInfo *deviceInfo, QWidget *parent) :
	QWidget(parent),
    ui(new Ui::ContentPane),
    deviceInfo(deviceInfo),
    selectedUid(uid),
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
    promptDialog = new PromptDialog(serviceInterface, deviceInfo->biotype,
                                    deviceInfo->device_id, selectedUid, this);
    promptDialog->enroll(deviceInfo->device_id, selectedUid, freeIndex, indexName);
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

    promptDialog = new PromptDialog(serviceInterface, deviceInfo->biotype,
                                    deviceInfo->device_id, selectedUid, this);
    promptDialog->verify(deviceInfo->device_id, uid, verifyIndex);

    delete promptDialog;
}


/**
 * @brief 生物特征搜索
 */
void ContentPane::on_btnSearch_clicked()
{
    promptDialog = new PromptDialog(serviceInterface, deviceInfo->biotype,
                                    deviceInfo->device_id, selectedUid, this);
    promptDialog->search(deviceInfo->device_id, selectedUid, 0, -1);

    delete promptDialog;
}
