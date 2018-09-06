#include <PolkitQt1/Subject>
#include "PolkitListener.h"
#include "Globals.h"
#include <QApplication>
#include <QTranslator>
#include <QDebug>
#include "mainwindow.h"


QString logPrefix;
void outputMessage(QtMsgType type, const QMessageLogContext &context,
           const QString &msg);

int main(int argc, char *argv[])
{
    logPrefix = "[ukui-polkit]:";
    qInstallMessageHandler(outputMessage);
	qDebug() << "Polkit Agent Started";

	QApplication agent(argc, argv);

    QString locale = QLocale::system().name();
    qDebug() << "Language: " <<locale;
    QTranslator translator_main, translator_bio;
    QString qmfile_main = QString("%1/i18n_qm/%2.qm").arg(GET_STR(INSTALL_PATH)).arg(locale);
    QString qmfile_bio = QString("%1/i18n_qm/%2.qm").arg(GET_STR(UKUI_BIOMETRIC)).arg(locale);
    translator_main.load(qmfile_main);
    translator_bio.load(qmfile_bio);
    agent.installTranslator(&translator_main);
    agent.installTranslator(&translator_bio);

    /* Run forever */
    agent.setQuitOnLastWindowClosed(false);
    PolkitListener listener;
    PolkitQt1::UnixSessionSubject session(getpid());
    if (listener.registerListener(session, POLKIT_LISTENER_ID)) {
        qDebug() << "Listener" << POLKIT_LISTENER_ID << "registered.";
    } else {
        qDebug() << "Could not register listener"
                    << POLKIT_LISTENER_ID << "Aborting";
        return EXIT_FAILURE;
    }

	agent.exec();
	return EXIT_SUCCESS;
}

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    QDateTime dateTime = QDateTime::currentDateTime();
    QByteArray time = QString("[%1] ").arg(dateTime.toString("MM-dd hh:mm:ss.zzz")).toLocal8Bit();
    QByteArray localMsg = msg.toLocal8Bit();
    QByteArray prefix = logPrefix.toLocal8Bit();
    switch(type) {
    case QtDebugMsg:
        fprintf(stderr, "%s %s [Debug]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        break;
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    case QtInfoMsg:
        fprintf(stderr, "%s %s [Info]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        break;
#endif
    case QtWarningMsg:
        fprintf(stderr, "%s %s [Warnning]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s %s [Critical]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s %s [Fatal]: %s\n", prefix.constData(),
                time.constData(), localMsg.constData());
        abort();
    }
}
