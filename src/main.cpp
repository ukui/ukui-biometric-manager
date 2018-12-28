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
#include <QApplication>
#include <QTranslator>
#include <QDir>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "servicemanager.h"
#include "messagedialog.h"

#define WORKING_DIRECTORY "/usr/share/biometric-manager"

void parseArguments(QApplication &app, QMap<QString, QString> &argMap)
{
	QApplication::setApplicationName("Biometric Manager");
    QApplication::setWindowIcon(QIcon(":/images/assets/logo.png"));
	QCommandLineParser parser;
	parser.addHelpOption();
	QString username;
	QCommandLineOption usernameOption(QStringList() << "u"
					<< "username", QObject::tr("Username"),
					"User Name", "");
	parser.addOption(usernameOption);
	parser.process(app);
	username = parser.value(usernameOption);
	argMap.insert("username", username);
//	qDebug() << "GUI:" << "Get username from command line - " << username;
}

void checkIsRunning()
{
    int fd, len;
    char buf[32];
    struct flock lock;

    const QString PID_DIR = QString("/var/run/user/%1").arg(QString::number(getuid()));
    const QString PID_FILE = PID_DIR + "/biometric-manager.pid";

    qDebug() << PID_DIR;
    QDir dir(PID_DIR);
    if(!dir.exists()) {
        if(!dir.mkdir(PID_DIR.toLocal8Bit().data())) {
            perror("create pid directory failed");
            exit(1);
        }
    }
    if( (fd = open(PID_FILE.toLocal8Bit().data(),
                   O_RDWR | O_CREAT, 0666)) == -1){
        perror("open pid file failed");
        exit(1);
    }

    memset(&lock, 0, sizeof(struct flock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;

    if(fcntl(fd, F_SETLK, &lock) < 0) {
//        perror("fcntl F_SETLK failed");
        printf("There is already an instance running\n");
        exit(1);
    }

    len = snprintf(buf, sizeof(buf), "%d", getpid());
    ftruncate(fd, 0);
    if(write(fd, buf, len) != len) {
        perror("write pid to lock file failed");
        exit(1);
    }
}
int main(int argc, char *argv[])
{
    checkIsRunning();

	QApplication a(argc, argv);

	/* 对中文环境安装翻译 */
	QString locale = QLocale::system().name();
	QTranslator translator;
    QString qmFile = QString(WORKING_DIRECTORY"/i18n_qm/%1.qm").arg(locale);
    translator.load(qmFile);
    a.installTranslator(&translator);
    qDebug() << "load translation file " << qmFile;

	/* 解析命令行参数 */
	QMap<QString, QString> argMap;
	parseArguments(a, argMap);

    ServiceManager *sm = ServiceManager::instance();

    if(!sm->serviceExists())
    {
        MessageDialog msgDialog(MessageDialog::Error,
                                QObject::tr("Fatal Error"),
                                QObject::tr("the biometric-authentication service was not started"));
        msgDialog.exec();
        exit(EXIT_FAILURE);
    }

    if(!sm->apiCompatible())
    {
        MessageDialog msgDialog(MessageDialog::Error,
                                QObject::tr("Fatal Error"),
                                QObject::tr("API version is not compatible"));
        msgDialog.exec();
        exit(EXIT_FAILURE);
    }

    MainWindow w(argMap.value("username"));
    w.setObjectName("MainWindow");
    w.show();
    QObject::connect(sm, &ServiceManager::serviceStatusChanged,
                     &w, &MainWindow::onServiceStatusChanged);

	return a.exec();
}
