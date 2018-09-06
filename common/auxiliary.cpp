#include <QDateTime>
#include <QDebug>
#include <assert.h>
#include <stdio.h>

/**
 * @brief QString 转 char * 指针
 * @param string
 * @return char *
 */
char *convertToCharPointer(QString string)
{
	QByteArray array = string.toLocal8Bit();
	char *tmp, *ptr;
	tmp = array.data();
	assert(tmp != 0);
	ptr = (char *)malloc(strlen(tmp)+1);
	strcpy(ptr, tmp);
	return ptr;
}

/**
 * @brief 自定义日志处理函数
 * @param type
 * @param context
 * @param msg
 */
QString logPrefix = "";
void loggerHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg_orig)
{
	(void)context;
	const char *logfile = "/tmp/bio.log";
	FILE *fd = fopen(logfile, "a+");
	char *datetime = convertToCharPointer(QDateTime::currentDateTime().
						toString("yyyy-MM-dd HH:mm:ss"));
	QString msg;
	if (logPrefix == "")
		msg = msg_orig;
	else
		msg = logPrefix + " " + msg_orig;
	if (!fd) {
		fprintf(stderr, "%s - Open log file failed\n", datetime);
		fprintf(stderr, "%s - %s\n", datetime, convertToCharPointer(msg));
	}
	switch (type) {
	case QtDebugMsg:
#ifndef DISABLE_LOG_TO_SCREEN
		fprintf(stderr, "%s - %s\n", datetime, convertToCharPointer(msg));
#endif
		fprintf(fd, "%s - %s\n", datetime, convertToCharPointer((msg)));
		break;
	default:
#ifndef DISABLE_LOG_TO_SCREEN
		fprintf(stderr, "%s - {MsgType:%d} %s\n", datetime, type, convertToCharPointer(msg));
#endif
		fprintf(fd, "%s - {MsgType:%d} %s\n", datetime, type, convertToCharPointer((msg)));
	}
	fclose(fd);
}
