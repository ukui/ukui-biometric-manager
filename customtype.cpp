#include "customtype.h"
#include <QDBusMetaType>

void registerCustomTypes()
{
	qDBusRegisterMetaType<DeviceInfo>();
	qDBusRegisterMetaType<BiometricInfo>();
	qDBusRegisterMetaType<QList<QDBusVariant> >();
}

/* For the type DeviceInfo */
QDBusArgument &operator<<(QDBusArgument &argument, const DeviceInfo &deviceInfo)
{
    argument.beginStructure();
    argument << deviceInfo.driver_id << deviceInfo.device_shortname
		<< deviceInfo.device_fullname << deviceInfo.enable
		<< deviceInfo.biotype << deviceInfo.stotype
		<< deviceInfo.eigtype << deviceInfo.vertype
		<< deviceInfo.idtype << deviceInfo.bustype
		<< deviceInfo.dev_status << deviceInfo.ops_status;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, DeviceInfo &deviceInfo)
{
    argument.beginStructure();
    argument >> deviceInfo.driver_id >> deviceInfo.device_shortname
		>> deviceInfo.device_fullname >> deviceInfo.enable
		>> deviceInfo.biotype >> deviceInfo.stotype
		>> deviceInfo.eigtype >> deviceInfo.vertype
		>> deviceInfo.idtype >> deviceInfo.bustype
		>> deviceInfo.dev_status >> deviceInfo.ops_status;
    argument.endStructure();
    return argument;
}

/* For the type BiometricInfo */
QDBusArgument &operator<<(QDBusArgument &argument, const BiometricInfo &biometricInfo)
{
    argument.beginStructure();
    argument << biometricInfo.uid << biometricInfo.biotype
		<< biometricInfo.driver_shortname << biometricInfo.index
		<< biometricInfo.index_name;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, BiometricInfo &biometricInfo)
{
    argument.beginStructure();
    argument >> biometricInfo.uid >> biometricInfo.biotype
		>> biometricInfo.driver_shortname >> biometricInfo.index
		>> biometricInfo.index_name;
    argument.endStructure();
    return argument;
}
