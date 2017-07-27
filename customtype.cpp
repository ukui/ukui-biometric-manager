#include "customtype.h"
#include <QDBusMetaType>

void registerCustomTypes()
{
	qDBusRegisterMetaType<DeviceInfo>();
	qDBusRegisterMetaType<QList<QDBusVariant> >();
}

QDBusArgument &operator<<(QDBusArgument &argument, const DeviceInfo &deviceInfo)
{
    argument.beginStructure();
    argument << deviceInfo.drv_ID << deviceInfo.name << deviceInfo.full_name
		<< deviceInfo.enable << deviceInfo.biotype << deviceInfo.stotype
		<< deviceInfo.eigtype << deviceInfo.vertype << deviceInfo.idtype
		<< deviceInfo.bustype << deviceInfo.dev_status
		<< deviceInfo.ops_status;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DeviceInfo &deviceInfo)
{
    argument.beginStructure();
    argument >> deviceInfo.drv_ID >> deviceInfo.name >> deviceInfo.full_name
		>> deviceInfo.enable >> deviceInfo.biotype >> deviceInfo.stotype
		>> deviceInfo.eigtype >> deviceInfo.vertype >> deviceInfo.idtype
		>> deviceInfo.bustype >> deviceInfo.dev_status
		>> deviceInfo.ops_status;
    argument.endStructure();
    return argument;
}
