#include "messagedialog.h"
#include "ui_messagedialog.h"
#include <QFile>

MessageDialog::MessageDialog(int type, const QString &title, const QString &msg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MessageDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    ui->btnCancel->hide();

    if(type == Error)
        ui->lblMessage->setStyleSheet("color:red");
    else if(type == Question)
        ui->btnCancel->show();

    setTitle(title);
    setMessage(msg);


    QFile qssFile(":/css/assets/promptdialog.qss");
    qssFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(qssFile.readAll());
    this->setStyleSheet(styleSheet);
    qssFile.close();

    ui->btnClose->setIcon(QIcon(":/images/assets/close.png"));

    connect(ui->btnClose, &QPushButton::clicked, this, &MessageDialog::close);
    connect(ui->btnOK, &QPushButton::clicked, this, &MessageDialog::accept);
    connect(ui->btnCancel, &QPushButton::clicked, this, &MessageDialog::reject);
}

MessageDialog::~MessageDialog()
{
    delete ui;
}

void MessageDialog::setTitle(const QString &text)
{
    ui->lblTitle->setText(text);
}

void MessageDialog::setMessage(const QString &text)
{
    ui->lblMessage->setText(text);
    ui->lblMessage->adjustSize();
    ui->lblMessage->setWordWrap(true);
}

void MessageDialog::setOkText(const QString &text)
{
    ui->btnOK->setText(text);
}

void MessageDialog::setCancelText(const QString &text)
{
    ui->btnCancel->setText(text);
}
