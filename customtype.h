#ifndef CUSTOMTYPE_H
#define CUSTOMTYPE_H

#include <QDBusArgument>

struct DeviceInfo {
	int driver_id;
	QString device_shortname;
	QString device_fullname;
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

struct BiometricInfo {
	int uid;
	int biotype;
	QString driver_shortname;
	int index;
	QString index_name;
};

Q_DECLARE_METATYPE(DeviceInfo)
Q_DECLARE_METATYPE(BiometricInfo)
Q_DECLARE_METATYPE(QList<QDBusVariant>)

void registerCustomTypes();
QDBusArgument &operator<<(QDBusArgument &argument, const DeviceInfo &deviceInfo);
const QDBusArgument &operator>>(const QDBusArgument &argument, DeviceInfo &deviceInfo);
QDBusArgument &operator<<(QDBusArgument &argument, const BiometricInfo &biometricInfo);
const QDBusArgument &operator>>(const QDBusArgument &argument, BiometricInfo &biometricInfo);

#endif // CUSTOMTYPE_H
