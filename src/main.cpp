#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>

#define WORKING_DIRECTORY "/usr/share/biometric-manager"

void parseArguments(QApplication &app, QMap<QString, QString> &argMap)
{
	QApplication::setApplicationName("Biometric Manager");
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
	w.show();

	return a.exec();
}
