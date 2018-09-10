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
	qDebug() << "GUI:" << "Get username from command line - " << username;
}

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	/* 对中文环境安装翻译 */
	QString locale = QLocale::system().name();
	QTranslator translator;
	if(locale == "zh_CN") {
		translator.load(WORKING_DIRECTORY"/i18n_qm/zh_CN.qm");
		a.installTranslator(&translator);
	}

	/* 解析命令行参数 */
	QMap<QString, QString> argMap;
	parseArguments(a, argMap);
    MainWindow w(argMap.value("username"));
    w.setObjectName("MainWindow");
    w.show();

	return a.exec();
}
