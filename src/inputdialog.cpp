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
#include "inputdialog.h"
#include "ui_inputdialog.h"
#include <QFile>

InputDialog::InputDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InputDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

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

void InputDialog::setError(const QString &text)
{
    ui->lblError->setText(text);
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
    Q_EMIT dataChanged(ui->lineEdit->text());
}

void InputDialog::on_lineEdit_returnPressed()
{
    Q_EMIT dataChanged(ui->lineEdit->text());
}
