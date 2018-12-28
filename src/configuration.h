#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QObject>

class Configuration : public QObject
{
    Q_OBJECT
private:
    explicit Configuration(QObject *parent = nullptr);
    Configuration(const Configuration &config) = delete;
    Configuration& operator =(const Configuration &rhs) = delete;

public:
    static Configuration *instance();
    QString getDefaultDevice();
    void setDefaultDevice(const QString &deviceName);

private:
    static QString configFile;
    static Configuration *instance_;

signals:
    void defaultDeviceChanged(const QString &deviceName);
};

#endif // CONFIGURATION_H
