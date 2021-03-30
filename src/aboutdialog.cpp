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
#include <QDesktopServices>
AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Biometric Manager"));
    ui->AboutIconLabel->setPixmap(QIcon::fromTheme("biometric-manager").pixmap(QSize(96,96)));
    QFont font;
    font.setPointSize(18);
    ui->AboutNameLabel->setFont(font);
    ui->AboutNameLabel->setText(tr("Biometric Manager"));
    ui->AboutVersionLabel->setText(tr("Version number: ") + get_current_version());
    ui->AboutBriefTextedit->setText(tr("         Biometric Manager is a supporting software for managing biometric identification which is developed by Kylin team.  It mainly contains biometirc verification management, biometirc service management, biometric device's driver management and biometirc features management, etc."
                                                                     "All functions of the software are still being perfected. Please look forward to it. "));
    ui->AboutVersionLabel->setStyleSheet("color:#595959;");
//    ui->AboutDevelopTextedit->setText(tr("Service & Support: ") + "support@kylinos.cn");
    ui->AboutDevelopTextedit->setText(tr("Service & Support: ") +
                                "<a href=\"mailto://support@kylinos.cn\""
                                "style=\"color:#595959\">"
                                "support@kylinos.cn</a>");
    connect(ui->AboutDevelopTextedit, &QLabel::linkActivated, this, [=](const QString url){
         QDesktopServices::openUrl(QUrl(url));
     });
    ui->AboutDevelopTextedit->setContextMenuPolicy(Qt::NoContextMenu);

    ui->AboutBriefTextedit->setText(tr("         Biometric Manager is a supporting software for managing biometric identification which is developed by Kylin team.  It mainly contains biometirc verification management, biometirc service management, biometric device's driver management and biometirc features management, etc."
                                                                     "All functions of the software are still being perfected. Please look forward to it. "));
  
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
