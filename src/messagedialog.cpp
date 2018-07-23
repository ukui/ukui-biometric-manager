#include "messagedialog.h"
#include "ui_messagedialog.h"
#include <QFile>

MessageDialog::MessageDialog(int type, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MessageDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);

    ui->btnOK->hide();
    ui->btnCancel->hide();

    if(type == Error)
        ui->lblMessage->setStyleSheet("color:red");
    else if(type == Question){
        ui->btnOK->show();
        ui->btnCancel->show();
    }

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
}
