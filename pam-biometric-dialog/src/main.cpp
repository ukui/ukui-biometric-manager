#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>
#include "generic.h"

int main(int argc, char *argv[])
{
    QApplication::setSetuidAllowed(true);
    QApplication a(argc, argv);

    QString locale = QLocale::system().name();
    qDebug() << "Language: " <<locale;
    QTranslator translator;
    QString qmfile = QString("%1/i18n_qm/%2.qm").arg(GET_STR(UKUI_BIOMETRIC)).arg(locale);
    translator.load(qmfile);
    a.installTranslator(&translator);

    MainWindow w;
    w.show();

    return a.exec();
}
