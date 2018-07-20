#include "inputdialog.h"
#include "ui_inputdialog.h"
#include <QFile>

InputDialog::InputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InputDialog)
{
    ui->setupUi(this);
    setWindowFlag(Qt::FramelessWindowHint);

    QFile qssFile(":/css/assets/promptdialog.qss");
    qssFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(qssFile.readAll());
    this->setStyleSheet(styleSheet);
    qssFile.close();

    ui->btnClose->setIcon(QIcon(":/images/assets/close.png"));
    ui->lineEdit->setFocus();
}

InputDialog::~InputDialog()
{
    delete ui;
}

void InputDialog::setTitle(const QString &text)
{
    ui->lblTitle->setText(text);
}

void InputDialog::setPrompt(const QString &text)
{
    ui->lblPrompt->setText(text);
}

void InputDialog::setText(const QString &text)
{
    ui->lineEdit->setText(text);
}

QString InputDialog::getText()
{
    return ui->lineEdit->text();
}

void InputDialog::on_btnClose_clicked()
{
    reject();
}

void InputDialog::on_btnOK_clicked()
{
    accept();
}

void InputDialog::on_lineEdit_returnPressed()
{
    accept();
}
