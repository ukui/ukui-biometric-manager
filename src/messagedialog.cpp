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

void MessageDialog::setMessageList(const QStringList &textList)
{
    ui->lblMessage->setAlignment(Qt::AlignLeft);

    for(auto text : textList) {
        QLabel *label = new QLabel(text, this);
        label->setFont(QFont("Sans Serif", 10));
        ui->messageLayout->addWidget(label);
    }
}
