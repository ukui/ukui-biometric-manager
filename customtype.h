#ifndef CUSTOMTYPE_H
#define CUSTOMTYPE_H

#include <QDBusArgument>

struct DeviceInfo {
	int drv_ID;
	QString name;
	QString full_name;
	int enable;
	int biotype;
	int stotype;
	int eigtype;
	int vertype;
	int idtype;
	int bustype;
	int dev_status;
	int ops_status;
};

Q_DECLARE_METATYPE(DeviceInfo)
Q_DECLARE_METATYPE(QList<QDBusVariant>)

void registerCustomTypes();
QDBusArgument &operator<<(QDBusArgument &argument, const DeviceInfo &deviceInfo);
const QDBusArgument &operator>>(const QDBusArgument &argument, DeviceInfo &deviceInfo);

#endif // CUSTOMTYPE_H
