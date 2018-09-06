#ifndef BIODEVICESWIDGET_H
#define BIODEVICESWIDGET_H

#include <QWidget>
#include "biodevices.h"

namespace Ui {
class BioDevicesWidget;
}

class BioDevicesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BioDevicesWidget(QWidget *parent = 0);
    ~BioDevicesWidget();
    void init();

signals:
    void back();
    void deviceChanged(const DeviceInfo &device);

private slots:
    void on_btnBack_clicked();
    void on_btnOK_clicked();
    void on_lwDevices_doubleClicked(const QModelIndex &);
    void on_cmbDeviceTypes_currentIndexChanged(int index);

    void onDeviceCountChanged();

private:
    Ui::BioDevicesWidget *ui;
    BioDevices bioDevices;
    QMap<int, QList<DeviceInfo>> devicesMap;
};

#endif // BIODEVICESWIDGET_H
