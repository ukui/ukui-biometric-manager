#include "customtype.h"
#include <QDBusMetaType>

void registerCustomTypes()
{
	qDBusRegisterMetaType<DeviceInfo>();
    qDBusRegisterMetaType<FeatureInfo>();
	qDBusRegisterMetaType<QList<QDBusVariant> >();
    qDBusRegisterMetaType<SearchResult>();
}

/* For the type DeviceInfo */
QDBusArgument &operator<<(QDBusArgument &argument, const DeviceInfo &deviceInfo)
{
    argument.beginStructure();
    argument << deviceInfo.device_id << deviceInfo.device_shortname
		<< deviceInfo.device_fullname << deviceInfo.driver_enable
		<< deviceInfo.device_available
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
    argument >> deviceInfo.device_id >> deviceInfo.device_shortname
		>> deviceInfo.device_fullname >> deviceInfo.driver_enable
		>> deviceInfo.device_available
		>> deviceInfo.biotype >> deviceInfo.stotype
		>> deviceInfo.eigtype >> deviceInfo.vertype
		>> deviceInfo.idtype >> deviceInfo.bustype
		>> deviceInfo.dev_status >> deviceInfo.ops_status;
    argument.endStructure();
    return argument;
}

/* For the type FeatureInfo */
QDBusArgument &operator<<(QDBusArgument &argument, const FeatureInfo &featureInfo)
{
    argument.beginStructure();
    argument << featureInfo.uid << featureInfo.biotype
        << featureInfo.device_shortname << featureInfo.index
        << featureInfo.index_name;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, FeatureInfo &featureInfo)
{
    argument.beginStructure();
    argument >> featureInfo.uid >> featureInfo.biotype
        >> featureInfo.device_shortname >> featureInfo.index
        >> featureInfo.index_name;
    argument.endStructure();
    return argument;
}

/* For the type SearchResult */
QDBusArgument &operator<<(QDBusArgument &argument, const SearchResult &ret)
{
    argument.beginStructure();
    argument << ret.uid << ret.index << ret.indexName;
    argument.endStructure();
    return argument;
}

const QDBusArgument &operator>>(const QDBusArgument &argument, SearchResult &ret)
{
    argument.beginStructure();
    argument >> ret.uid >> ret.index >> ret.indexName;
    argument.endStructure();
    return argument;
}

