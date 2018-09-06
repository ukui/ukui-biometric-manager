#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int enable_debug;
char *log_prefix;

void logger(char *format, ...)
{
    if(!enable_debug){
        return;
    }

	va_list args;

	time_t t = time(NULL);
	char timestr[32] = {0};
	strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", localtime(&t)); //产生"YYYYMMDD hh:mm:ss"格式的字符串。

    fprintf(stderr, "[%s] %s - ", log_prefix, timestr);
	va_start(args, format); /* 初始化 args */
	vfprintf(stderr, format, args);
}
