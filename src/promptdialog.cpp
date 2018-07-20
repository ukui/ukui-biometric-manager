#include "promptdialog.h"
#include "ui_promptdialog.h"
#include <QMovie>
#include <QPushButton>
#include <QDebug>
#include <QKeyEvent>
#include <QStandardItemModel>
#include <pwd.h>

PromptDialog::PromptDialog(QDBusInterface *service,  int bioType,
                           int deviceId, int uid, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::PromptDialog),
      serviceInterface(service),
      type(bioType),
      deviceId(deviceId),
      uid(uid),
      ops(IDLE),
      opsResult(UNDEFINED)
{
	ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    ui->btnClose->setFlat(true);
    ui->btnClose->setIcon(QIcon(":/images/assets/close.png"));

	/* 设置 CSS */
	QFile qssFile(":/css/assets/promptdialog.qss");
	qssFile.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(qssFile.readAll());
	this->setStyleSheet(styleSheet);
	qssFile.close();
    ui->treeViewResult->hide();

    movie = new QMovie(getGif(type));
    ui->lblImage->setMovie(movie);

    connect(serviceInterface, SIGNAL(StatusChanged(int,int)),
            this, SLOT(onStatusChanged(int,int)));
}

PromptDialog::~PromptDialog()
{
	delete ui;
}

void PromptDialog::setTitle(int opsType)
{
    QString title = EnumToString::transferBioType(type);
    switch(opsType) {
    case ENROLL:
        title += tr("Enroll");
        break;
    case VERIFY:
        title += tr("Verify");
        break;
    case SEARCH:
        title += tr("Search");
        break;
    }

    ui->lblTitle->setText(title);
}

/**
 * @brief 设置要显示的信息
 * @param text
 */
void PromptDialog::setPrompt(const QString &text)
{
    ui->lblPrompt->setText(text);
    ui->lblPrompt->setWordWrap(true);
    ui->lblPrompt->adjustSize();
}

void PromptDialog::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape) {
        if(ui->btnClose->isEnabled())
            accept();
        return;
    }
}

void PromptDialog::on_btnClose_clicked()
{
    QList<QVariant> args;
    args << QVariant(deviceId) << QVariant(5);
    serviceInterface->callWithCallback("StopOps", args, this,
                        SLOT(StopOpsCallBack(const QDBusMessage &)),
                        SLOT(errorCallBack(const QDBusError &)));
    setPrompt(tr("In progress, please wait..."));
}

PromptDialog::Result PromptDialog::getResult()
{
    return opsResult;
}

void PromptDialog::setSearchResult(bool isAdmin, const QList<SearchResult> &searchResultList)
{
    QStandardItemModel *model = new QStandardItemModel(ui->treeViewResult);
    if(isAdmin)
        model->setHorizontalHeaderLabels(QStringList{"    " + tr("Index"), tr("UserName"), tr("FeatureName")});
    else
        model->setHorizontalHeaderLabels(QStringList{"    " + tr("Index"), tr("FeatureName")});

    for(int i = 0; i < searchResultList.size(); i++) {
        SearchResult ret = searchResultList[i];
        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::number(i+1)));
        if(isAdmin) {
            struct passwd *pwd = getpwuid(ret.uid);
            row.append(new QStandardItem(QString(pwd->pw_name)));
        }
        row.append(new QStandardItem(ret.indexName));
        model->appendRow(row);
    }

    ui->treeViewResult->setModel(model);
    ui->treeViewResult->show();
    this->setFixedHeight(height() + 100);
}

QString PromptDialog::getGif(int type)
{
    switch(type) {
    case BIOTYPE_FINGERPRINT:
        return ":/images/assets/fingerprint.gif";
    case BIOTYPE_FINGERVEIN:
        return ":/images/assets/fingervein.gif";
    case BIOTYPE_IRIS:
        return ":/images/assets/iris.gif";
    case BIOTYPE_VOICEPRINT:
        return ":/images/assets/voiceprint.gif";
    }
    return QString();
}

QString PromptDialog::getImage(int type)
{
    switch(type) {
    case BIOTYPE_FINGERPRINT:
        return ":/images/assets/fingerprint.png";
    case BIOTYPE_FINGERVEIN:
        return ":/images/assets/fingervein.png";
    case BIOTYPE_IRIS:
        return ":/images/assets/iris.png";
    case BIOTYPE_VOICEPRINT:
        return ":/images/assets/voiceprint.png";
    }
    return QString();
}


int PromptDialog::enroll(int drvId, int uid, int idx, const QString &idxName)
{
    QList<QVariant> args;
    args << drvId << uid << idx << idxName;

    this->setTitle(ENROLL);
    this->setPrompt(tr("Permission is required.\n"
                       "Please authenticate yourself to continue"));
    ui->btnClose->setEnabled(false);

    /*
     * 异步回调参考资料：
     * https://github.com/RalfVB/SQEW-OS/blob/master/src/module/windowmanager/compton.cpp
     */
    serviceInterface->callWithCallback("Enroll", args, this,
                        SLOT(enrollCallBack(const QDBusMessage &)),
                        SLOT(errorCallBack(const QDBusError &)));
    ops = ENROLL;

    movie->start();
    return exec();
}

void PromptDialog::enrollCallBack(const QDBusMessage &reply)
{
    int result;
    result = reply.arguments()[0].value<int>();
    qDebug() << "Enroll result: " << result;

    ui->btnClose->setEnabled(true);

    switch(result) {
    case DBUS_RESULT_SUCCESS: { /* 录入成功 */
        opsResult = SUCESS;
        setPrompt(tr("Enroll successfully"));
        break;
    }
    default:
        opsResult = ERROR;
        handleErrorResult(result);
        break;
    }
    ops = IDLE;
    showClosePrompt();
}

int PromptDialog::verify(int drvId, int uid, int idx)
{
    QList<QVariant> args;
    args << drvId << uid << idx;

    this->setTitle(VERIFY);

    serviceInterface->callWithCallback("Verify", args, this,
                        SLOT(verifyCallBack(const QDBusMessage &)),
                        SLOT(errorCallBack(const QDBusError &)));
    ops = VERIFY;

    movie->start();
    return exec();
}

void PromptDialog::verifyCallBack(const QDBusMessage &reply)
{
    int result;
    result = reply.arguments()[0].value<int>();
    qDebug() << "Verify result: " << result;

    if(result >= 0) {
        opsResult = SUCESS;
        setPrompt(tr("Verify successfully"));
    } else if(result == DBUS_RESULT_NOTMATCH) {
        setPrompt(tr("Not Match"));
    } else {
        handleErrorResult(result);
    }

    ops = IDLE;
    showClosePrompt();
}

int PromptDialog::search(int drvId, int uid, int idxStart, int idxEnd)
{
    QList<QVariant> args;
    args << drvId << uid << idxStart << idxEnd;

    this->setTitle(SEARCH);

    serviceInterface->callWithCallback("Search", args, this,
                        SLOT(searchCallBack(const QDBusMessage &)),
                        SLOT(errorCallBack(const QDBusError &)));

    ops = SEARCH;

    movie->start();
    return exec();
}

void PromptDialog::searchCallBack(const QDBusMessage &reply)
{
    int result;
    result = reply.arguments()[0].value<int>();
    qDebug() << "Verify result: " << result;

    if(result > 0) {
        setPrompt(tr("Search Result"));
        int count  = result;
        QDBusArgument argument = reply.arguments().at(1).value<QDBusArgument>();
        QList<QVariant> variantList;
        argument >> variantList;
        QList<SearchResult> results;
        for(int i = 0; i < count; i++) {
            QDBusArgument arg =variantList.at(i).value<QDBusArgument>();
            SearchResult ret;
            arg >> ret;
            results.append(ret);
        }
        this->setSearchResult(isAdmin(uid), results);
        opsResult = SUCESS;
    }
    else if(result >= DBUS_RESULT_NOTMATCH)
        setPrompt(tr("No matching features Found"));
    else
        handleErrorResult(result);

    ui->lblImage->setPixmap(getImage(type));
}

void PromptDialog::StopOpsCallBack(const QDBusMessage &reply)
{
    int ret = reply.arguments().at(0).toInt();
    if(ret == 0) {
        qDebug() << "Stop Successfully";
        accept();
    }
}

void PromptDialog::errorCallBack(const QDBusError &error)
{
    qDebug() << "DBus Error: " << error.message();
}

void PromptDialog::onStatusChanged(int drvId, int statusType)
{
    if (!(drvId == deviceId && statusType == STATUS_NOTIFY))
        return;

    ui->btnClose->setEnabled(true);

    //过滤掉当录入时使用生物识别授权接收到的认证的提示信息
    if(ops == ENROLL) {
        QDBusMessage reply = serviceInterface->call("UpdateStatus", drvId);
        if(reply.type() == QDBusMessage::ErrorMessage) {
            qDebug() << "DBUS: " << reply.errorMessage();
            return;
        }
        int devStatus = reply.arguments().at(3).toInt();
        qDebug() << devStatus;

        if(!(devStatus >= 201 && devStatus < 203)) {
            return;
        }
    }

    QDBusMessage notifyReply = serviceInterface->call("GetNotifyMesg", drvId);
    if(notifyReply.type() == QDBusMessage::ErrorMessage) {
        qDebug() << "DBUS: " << notifyReply.errorMessage();
        return;
    }
    QString prompt = notifyReply.arguments().at(0).toString();
    qDebug() << prompt;

    setPrompt(prompt);
}

void PromptDialog::handleErrorResult(int error)
{
    switch(error) {
    case DBUS_RESULT_ERROR: {
        //操作失败，需要进一步获取失败原因
        QDBusPendingReply<int, int, int, int, int, int> reply =
                serviceInterface->call("UpdateStatus", deviceId);
        reply.waitForFinished();
        if (reply.isError()) {
            qDebug() << "DBUS:" << reply.error();
            setPrompt(tr("D-Bus calling error"));
            return;
        }
        int opsStatus = reply.argumentAt(OPS_STATUS_INDEX).value<int>();
        opsStatus = opsStatus % 100;
        qDebug() << "Ops Error: " << opsStatus;
        switch(opsStatus) {
        case OPS_FAILED:
            setFailed();
            break;
        case OPS_ERROR:
            //设备底层发生了错误
            setPrompt(tr("Device encounters an error"));
            break;
        case OPS_CANCEL:
            //用户取消了操作，直接返回
            return;
        case OPS_TIMEOUT:
            //超时
            setPrompt(tr("Operation timeout"));
            break;;
        }
        break;
    }
    case DBUS_RESULT_DEVICEBUSY:
        //设备忙
        setPrompt(tr("Device is busy"));
        break;
    case DBUS_RESULT_NOSUCHDEVICE:
        //设备不存在
        setPrompt(tr("No such device"));
        break;
    case DBUS_RESULT_PERMISSIONDENIED:
        //没有权限
        setPrompt(tr("Permission denied"));
        break;
    }
}

void PromptDialog::setFailed()
{
    switch(ops) {
    case ENROLL:
        setPrompt(tr("Failed to enroll"));
        break;
    case VERIFY:
        setPrompt(tr("Failed to match"));
        break;
    case SEARCH:
        setPrompt(tr("Not Found"));
        break;
    default:
        break;
    }
}


void PromptDialog::showClosePrompt()
{
    ui->lblImage->setPixmap(getImage(type));
    QString prompt = QString("<font size = '4'>%1</font>").arg(ui->lblPrompt->text()) + "<br>" +
            tr("<font size='2'>the window will be closed after two second</font>");
    setPrompt(prompt);

    QTimer::singleShot(2000, this, [&]{accept();});
}
