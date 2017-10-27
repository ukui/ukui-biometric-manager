#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>

#define WORKING_DIRECTORY "/usr/local/BiometricManager"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

	/* 对中文环境安装翻译 */
	QString locale = QLocale::system().name();
	QTranslator translator;
	if(locale == "zh_CN") {
		translator.load(WORKING_DIRECTORY"/i18n/zh_CN.qm");
		a.installTranslator(&translator);
	}

	MainWindow w;
	w.show();

	return a.exec();
}
