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
#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QFile>
#include <QIcon>
#include <QDebug>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Biometric Manager"));
    ui->ksc_about_icon_label->setPixmap(QIcon::fromTheme("biometric-manager").pixmap(QSize(96,96)));
    QFont font;
    font.setPointSize(18);
    ui->ksc_security_center_about_name_label->setFont(font);
    ui->ksc_security_center_about_name_label->setText(tr("Biometric Manager"));
    ui->ksc_security_center_about_version_label->setText(tr("Version number: ") + get_current_version());
    ui->ksc_security_center_about_brief_textedit->setText(tr("         Biometric Manager is a supporting software for managing biometric identification which is developed by Kylin team.  It mainly contains biometirc verification management, biometirc service management, biometric device's driver management and biometirc features management, etc."
                                                                      "All functions of the software are still being perfected. Please look forward to it. "));
    ui->ksc_security_center_about_version_label->setStyleSheet("color:#595959;");
    ui->ksc_security_center_about_develop_textedit->setText(tr("developers：") + "\nliuyuanpeng@kylinos.cn");
    this->setBackgroundRole(QPalette::Base);
    this->setAutoFillBackground(true);
}

QString AboutDialog::get_current_version()
{
    FILE *pp = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char *q = NULL;
    QString version = "none";

    pp = popen("dpkg -l  ukui-biometric-manager | grep  ukui-biometric-manager", "r");
    if(NULL == pp)
        return version;

    while((read = getline(&line, &len, pp)) != -1){
        q = strrchr(line, '\n');
        *q = '\0';

        QString content = line;
        QStringList list = content.split(" ");

        list.removeAll("");

        if (list.size() >= 3)
            version = list.at(2);
    }

    free(line);
    pclose(pp);
    return version;
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
