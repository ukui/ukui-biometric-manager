/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
**/
#ifndef CUSTOMTYPE_H
#define CUSTOMTYPE_H

#include <QtDBus/QtDBus>

#define UNUSED(x) (void)x

#define ADMIN_UID 0

#define SERVICE "biometric-authentication.service"
#define DBUS_SERVICE "org.ukui.Biometric"
#define DBUS_PATH "/org/ukui/Biometric"
#define DBUS_INTERFACE "org.ukui.Biometric"

#define FD_DBUS_SERVICE     "org.freedesktop.DBus"
#define FD_DBUS_PATH        "/org/freedesktop/DBus"
#define FD_DBUS_INTERFACE   "org.freedesktop.DBus"

enum BioType {
	BIOTYPE_FINGERPRINT,
	BIOTYPE_FINGERVEIN,
	BIOTYPE_IRIS,
    BIOTYPE_FACE,
    BIOTYPE_VOICEPRINT,
	__MAX_NR_BIOTYPES
};


/* 录入/删除/搜索等 D-Bus 调用的最终结果，即返回值里的 result */
/*
verify接口：
>= 0，操作成功，并且验证通过；
< 0，-RetCode；

identify接口：
>= 0，操作成功，并且识别到；
< 0，-RetCode；

search接口：
>= 0，操作成功，搜索到特征，特征列表长度；
< 0，-RetCode；

get_feature_list接口：
>= 0，操作成功，获取的特征列表长度；
< 0，-RetCode；

enroll接口：
= 0，操作成功；
< 0，-RetCode；

capture接口：
= 0，操作成功；
< 0，-RetCode；
*/
enum DBusResult {
	DBUS_RESULT_SUCCESS = 0,
    DBUS_RESULT_NOTMATCH = -1,
    DBUS_RESULT_ERROR = -2,
    DBUS_RESULT_DEVICEBUSY = -3,
    DBUS_RESULT_NOSUCHDEVICE = -4,
    DBUS_RESULT_PERMISSIONDENIED = -5
};

/* 设备操作结果 ops_status，由 UpdateStatus 函数获得 */
#define OPS_STATUS_INDEX 4 /* Index of OpsStatus in UpdateStatus result */
enum OpsStatus {
	OPS_SUCCESS,
	OPS_FAILED,
	OPS_ERROR,
	OPS_CANCEL,
	OPS_TIMEOUT,
	__MAX_NR_OPSCODES
};

struct DeviceInfo {
	int device_id;
	QString device_shortname; /* aka driverName */
	QString device_fullname;
	int driver_enable; /* The corresponding driver is enabled/disabled */
	int device_available; /* The driver is enabled and the device is connected */
	int biotype;
	int stotype;
	int eigtype;
	int vertype;
	int idtype;
	int bustype;
	int dev_status;
	int ops_status;
};

struct FeatureInfo {
	int uid;
	int biotype;
	QString device_shortname;
	int index;
	QString index_name;
};

/* StatusChanged D-Bus 信号触发时的状态变化类型 */
enum StatusType {
    STATUS_DEVICE = 0,
	STATUS_OPERATION,
	STATUS_NOTIFY
};

Q_DECLARE_METATYPE(DeviceInfo)
Q_DECLARE_METATYPE(FeatureInfo)
Q_DECLARE_METATYPE(QList<QDBusVariant>)

void registerCustomTypes();
QDBusArgument &operator<<(QDBusArgument &argument, const DeviceInfo &deviceInfo);
const QDBusArgument &operator>>(const QDBusArgument &argument, DeviceInfo &deviceInfo);
QDBusArgument &operator<<(QDBusArgument &argument, const FeatureInfo &featureInfo);
const QDBusArgument &operator>>(const QDBusArgument &argument, FeatureInfo &featureInfo);

enum VerifyType {
	VERIFY_HARDWARE,
	VERIFY_SOFTWARE,
	VERIFY_MIX,
	VERIFY_OTHER
};

enum StorageType {
	STORAGE_DEVICE,
	STORAGE_OS,
	STORAGE_MIX
};

enum BusType {
	BUS_SERIAL,
	BUS_USB,
    BUS_PCIE,
    BUS_ANY = 100,
    BUS_OTHER
};

enum IdentifyType {
	IDENTIFY_HARDWARE,
	IDENTIFY_SOFTWARE,
	IDENTIFY_MIX,
	IDENTIFY_OTHER
};

struct SearchResult {
    int uid;
    int index;
    QString indexName;
};

Q_DECLARE_METATYPE(SearchResult)
QDBusArgument &operator<<(QDBusArgument &argument, const SearchResult &ret);
const QDBusArgument &operator>>(const QDBusArgument &argument, SearchResult &ret);

class EnumToString : public QObject
{
    Q_OBJECT
public:
    static QString transferBioType(int type);
    static QString transferVerifyType(int type);
    static QString transferStorageType(int type);
    static QString transferBusType(int type);
    static QString transferIdentifyType(int type);
};

bool isAdmin(int uid);

#endif // CUSTOMTYPE_H
