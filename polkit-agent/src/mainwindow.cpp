/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
**/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMovie>
#include <QFile>
#include <QDir>
#include <QPainter>
#include <QPixmap>
#include <QFontMetrics>
#include <QtMath>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <libintl.h>
#include <locale.h>

#include "generic.h"
#define _(string) gettext(string)

MainWindow::MainWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainWindow),
    enableBioAuth(false),
    receiveBioPAM(false),
    useDoubleAuth(false),
    isbioSuccess(false),
    authMode(UNDEFINED)
{
    ui->setupUi(this);
    setWindowTitle(tr("Authentication"));
    setWindowFlags(Qt::WindowStaysOnTopHint);

    widgetBioAuth = new BioAuthWidget(this);
    widgetBioDevices = new BioDevicesWidget(this);
    ui->formLayout->addWidget(widgetBioAuth);
    ui->formLayout->addWidget(widgetBioDevices);

    connect(widgetBioDevices, &BioDevicesWidget::deviceChanged,
            this, [&](const DeviceInfo &device){
        widgetBioAuth->startAuth(getUid(userName), device);
        switchWidget(BIOMETRIC);
    });

    connect(widgetBioDevices, &BioDevicesWidget::back,
            this, [&]{
        switchWidget(BIOMETRIC);
    });

    connect(&bioDevices, &BioDevices::deviceCountChanged,
            this, [&]{
        widgetBioAuth->setMoreDevices(bioDevices.count() > 1);
    });

    connect(widgetBioAuth, &BioAuthWidget::authComplete,
            this, [&](uid_t uid, bool ret){
        qDebug() << "biometric authentication complete: " << uid << ret;

        if(uid == getUid(userName) && ret)
        {
            if(useDoubleAuth){
                isbioSuccess = true;
                emit switchToBiometric();
                authMode = UNDEFINED;
            }else{
                accept(BIOMETRIC_SUCCESS);
            }
        }else if(useDoubleAuth){
            setMessage(tr("Authentication failed, please try again."));
           // widgetBioAuth->startAuth(getUid(userName), *device);
            if(!isbioSuccess){
                QTimer::singleShot(1000,this,SLOT(restart_bio_identify()));
            }
        }
    });

    connect(widgetBioAuth, &BioAuthWidget::selectDevice,
            this, [&]{
        widgetBioDevices->init(getUid(userName));
        switchWidget(DEVICES);
    });

    connect(widgetBioAuth, &BioAuthWidget::switchToPassword,
            this, [&] {
        accept(BIOMETRIC_IGNORE);
    });

    QFile qssFile(":/qss/src/main.qss");
    qssFile.open(QIODevice::ReadOnly);
    setStyleSheet(qssFile.readAll());
    qssFile.close();

    ui->cmbUsers->hide();
    ui->widgetDetails->hide();
    ui->btnDetails->setIcon(QIcon(":/image/assets/arrow_right.svg"));
    ui->btnDetails->hide();
    switchWidget(UNDEFINED);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::restart_bio_identify()
{
    DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
    if(device)
        widgetBioAuth->startAuth(getUid(userName), *device);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    widgetBioAuth->stopAuth();
    emit canceled();

    return QWidget::closeEvent(event);
}


/*** main ***/

void MainWindow::on_cmbUsers_currentTextChanged(const QString &userName)
{
    qDebug() << "user changed to " << userName;
    widgetBioAuth->stopAuth();	
    authMode=UNDEFINED;

    this->userName = userName;
    ui->lblMessage->clear();
    emit userChanged(userName);
}

int MainWindow::enable_biometric_authentication()
{
    char conf_file[] = GET_STR(CONFIG_FILE);
    FILE *file;
    char line[1024], is_enable[16];
    int i;


    if((file = fopen(conf_file, "r")) == NULL){
        return 0;
    }
    while(fgets(line, sizeof(line), file)) {
        i = sscanf(line, "EnableAuth=%s\n",  is_enable);
        if(i > 0) {
            break;
        }
    }

    fclose(file);
    if(!strcmp(is_enable, "true"))
        return 1;
    return 0;
}

void MainWindow::on_btnDetails_clicked()
{
    if(ui->widgetDetails->isHidden()) {
        ui->widgetDetails->show();
        ui->btnDetails->setIcon(QIcon(":/image/assets/arrow_down.svg"));
//        resize(width(), height() + ui->widgetDetails->height());
    }
    else {
        ui->widgetDetails->hide();
        ui->btnDetails->setIcon(QIcon(":/image/assets/arrow_right.svg"));
//        resize(width(), height() - ui->widgetDetails->height());
    }
    adjustSize();
}

void MainWindow::on_btnCancel_clicked()
{
    close();
}

void MainWindow::on_btnAuth_clicked()
{
    on_lePassword_returnPressed();
}

/*** pagePassword ***/

void MainWindow::on_lePassword_returnPressed()
{
    emit accept(ui->lePassword->text());
    switchWidget(UNDEFINED);
    setMessage(tr("in authentication, please wait..."));
}

void MainWindow::on_btnBioAuth_clicked()
{
    emit switchToBiometric();
    authMode = BIOMETRIC;
}

/*** end of control's slot ***/


/*** public member ***/

void MainWindow::setIcon(const QString &iconName)
{
    QIcon::setThemeName("ukui-icon-theme");
    if(!QIcon::hasThemeIcon("ukui-polkit")) {
        QDir iconsDir("/usr/share/icons");
        auto themesList = iconsDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        qDebug() << themesList;
        for(auto theme : themesList) {
            QIcon::setThemeName(theme);
            if(QIcon::hasThemeIcon("ukui-polkit")) {
                qDebug() << theme << "has ukui-polkit icon";
                break;
            }
        }
    }
    QPixmap icon = QIcon::fromTheme("ukui-polkit").pixmap(64, 64);
    QPixmap actionIcon = QIcon::fromTheme(iconName).pixmap(32, 32);
    QPainter painter;

    painter.begin(&icon);
    QRect rect(icon.width() - actionIcon.width(),
               icon.height() - actionIcon.height(),
               actionIcon.width(), actionIcon.height());
    painter.drawPixmap(rect, actionIcon);
    painter.end();

    setWindowIcon(icon);
    ui->lblIcon->setPixmap(icon);
}

void MainWindow::setHeader(const QString &text)
{
    ui->lblHeader->setText(text);
    ui->lblContent->setText(tr("An application is attempting to perform an action that requires privileges."
                            " Authentication is required to perform this action."));
}

void MainWindow::setUsers(const QStringList &usersList)
{
    if(usersList.isEmpty())
        return;

    if(usersList.size() == 1) {
        userName = usersList.at(0);
    }

    ui->cmbUsers->addItems(usersList);
    ui->cmbUsers->show();
}

void MainWindow::setDetails(const QString &subPid, const QString &callerPid, const QString &actionId,
                            const QString &actionDesc, const QString vendorName,
                            const QString vendorUrl)
{
    ui->lblSubjectPid->setText(subPid);
    ui->lblCallerPid->setText(callerPid);
    ui->lblActionId->setText(actionId);
    ui->lblActionDesc->setText(actionDesc);
    QString vendor = QString("<a href=\"%1\">%2").arg(vendorUrl).arg(vendorName);
    ui->lblVendor->setText(vendor);
}

void MainWindow::setPrompt(const QString &text, bool echo)
{
    QString prompt = text;

    if(text == "Password: "){
        prompt = tr("Password: ");
        if(useDoubleAuth){
            setMessage(tr("Please enter your password or enroll your fingerprint "));
        }
    }

    ui->lblPrompt->setText(prompt);
    ui->lePassword->setEchoMode(echo ? QLineEdit::Normal : QLineEdit::Password);
    switchWidget(PASSWORD);
}

QString MainWindow::check_is_pam_message(QString text)
{
    if(!(text.startsWith("Authenticated failed, ")&&text.endsWith(" login attemps left")) \
            &&!(text.startsWith("Account locked ")&&text.endsWith(" fail attempts")))
        return text;


    setlocale(LC_ALL,"");
    bindtextdomain("Linux-PAM","/usr/share/locale");
    bind_textdomain_codeset("Linux-PAM","UTF-8");
    textdomain("Linux-PAM");
    char*  str;
    QByteArray ba = text.toLatin1(); // must
    str=ba.data();

    int a,b;
    if(sscanf(str,"Authenticated failed, %d login attemps left",&a))
          sprintf(str,_("Authenticated failed, %d login attemps left"),a);

    if(sscanf(str,"Account locked %d minutes due to %d fail attempts",&a,&b))
          sprintf(str,_("Account locked %d minutes due to %d fail attempts"),a,b);
    qDebug()<<"str = "<<str;
    return QString(str);

}

void MainWindow::setMessage(const QString &text)
{
    QString message = this->check_is_pam_message(text);
    ui->lblMessage->setText(message);
}

void MainWindow::setAuthResult(bool result, const QString &text)
{
    QString message = text;

    if(!result && text.isEmpty())
        message = tr("Authentication failed, please try again.");

    if(authMode == PASSWORD)
        ui->lblMessage->setText(message);
//    else if(authMode == BIOMETRIC)
//        ui->lblBioNotify->setText(message);

    switchWidget(PASSWORD);
}

void MainWindow::clearEdit()
{
    ui->lePassword->setText("");
}

void MainWindow::switchAuthMode(Mode mode)
{
    if(useDoubleAuth && isbioSuccess){
        accept(BIOMETRIC_SUCCESS);
        return ;
    }

    enableBioAuth  = bioDevices.count() > 0;
    if(!enable_biometric_authentication()){
    	enableBioAuth = false;
    }
    int uid = getUid(userName);
    int devCount = bioDevices.getFeatureCount(uid);
    if(devCount < 1)
        enableBioAuth = false;

    if(mode == BIOMETRIC && enableBioAuth == false)
        useDoubleAuth = false;

    switch(mode){
    case PASSWORD:
    {
        qDebug() << "switch to password";

        authMode = mode;

        if(!enableBioAuth || !receiveBioPAM || useDoubleAuth) {
            ui->btnBioAuth->hide();
        }else{
            ui->btnBioAuth->show();
        }

        if(enableBioAuth){
            DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
            if(device){
                widgetBioAuth->startAuth(getUid(userName), *device);
            }
        }
    }
        break;
    case BIOMETRIC:
    {
        qDebug() << "switch to biometric";

        receiveBioPAM = true;

        if(authMode == PASSWORD) {
            emit accept(BIOMETRIC_IGNORE);
            return;
        }else if(!enableBioAuth){
            qDebug() << "It doesn't meet the condition for enabling biometric authentication, switch to password.";
            emit accept(BIOMETRIC_IGNORE);
            return;
        } else if(authMode == BIOMETRIC) {
            DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
            if(!device)
                device = bioDevices.getFirstDevice();
            if(device) {
                if(useDoubleAuth){
                    emit accept(BIOMETRIC_IGNORE);
                    widgetBioAuth->startAuth(getUid(userName), *device);
                    return;
                }else{
                    widgetBioAuth->startAuth(getUid(userName), *device);
                    widgetBioAuth->setMoreDevices(bioDevices.count() > 1);
                }
            }
            authMode = BIOMETRIC;
        } else if(authMode == UNDEFINED){
            authMode = BIOMETRIC;

            if(enableBioAuth) {
                qDebug() << "enable biometric authenticaion";
                DeviceInfo *device = bioDevices.getDefaultDevice(getUid(userName));
                if(device) {
                    if(useDoubleAuth){
                        emit accept(BIOMETRIC_IGNORE);
                        widgetBioAuth->startAuth(getUid(userName), *device);
                        return;
                    }else{
                        widgetBioAuth->startAuth(getUid(userName), *device);
                        widgetBioAuth->setMoreDevices(bioDevices.count() > 1);
                    }
                } else {
                    qDebug() << "No default device, switch to password";
                    emit accept(BIOMETRIC_IGNORE);
                    return;
                }

            } else {

                /* pass biometric's pam module if there are not available devices */
                qDebug() << "It doesn't meet the condition for enabling biometric authentication, switch to password.";
                emit accept(BIOMETRIC_IGNORE);
                return;
            }
        }
    }
        break;
    default:
        break;
    }
    switchWidget(mode);
}

/*** end of public memeber ***/


/*** private member ***/

uid_t MainWindow::getUid(const QString &userName)
{
    struct passwd *pwd = getpwnam(userName.toStdString().c_str());
    if(pwd == NULL) {
        qWarning() << "getpwnam error: " << strerror(errno);
        return -1;
    }
    return pwd->pw_uid;
}

void MainWindow::setDoubleAuth(bool val){
     useDoubleAuth = val;
}

void MainWindow::stopDoubleAuth()
{
    if(useDoubleAuth && widgetBioAuth)
        widgetBioAuth->stopAuth();
}

void MainWindow::switchWidget(Mode mode)
{
    ui->widgetPasswdAuth->hide();
    ui->btnAuth->hide();
    widgetBioAuth->hide();
    widgetBioDevices->hide();

    switch(mode){
    case PASSWORD:
        setMinimumWidth(420);
        ui->widgetPasswdAuth->show();
        ui->lePassword->setFocus();
        ui->lePassword->setAttribute(Qt::WA_InputMethodEnabled, false);
            ui->btnAuth->show();

        break;
    case BIOMETRIC:
        setMaximumWidth(380);
        widgetBioAuth->show();
        break;
    case DEVICES:
        widgetBioDevices->show();
    default:
        break;
    }
    adjustSize();
}

/*** end of private member ***/
