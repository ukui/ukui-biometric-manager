#include "messagedialog.h"
#include "ui_messagedialog.h"
#include <QFile>

MessageDialog::MessageDialog(int type, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MessageDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);

    if(type == Error)
        ui->lblMessage->setStyleSheet("color:red");

    QFile qssFile(":/css/assets/promptdialog.qss");
    qssFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(qssFile.readAll());
    this->setStyleSheet(styleSheet);
    qssFile.close();

    ui->btnClose->setIcon(QIcon(":/images/assets/close.png"));

    connect(ui->btnClose, &QPushButton::clicked, this, &MessageDialog::close);
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
