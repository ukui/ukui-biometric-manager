#ifndef BIOAUTHWIDGET_H
#define BIOAUTHWIDGET_H

#include <QWidget>
#include "bioauth.h"

namespace Ui {
class BioAuthWidget;
}

class BioAuthWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BioAuthWidget(QWidget *parent = 0);
    ~BioAuthWidget();

public slots:
    void startAuth(uid_t uid, const DeviceInfo &device);
    void setMoreDevices(bool hasMore);

signals:
    void switchToPassword();
    void selectDevice();
    void authComplete(uid_t uid, bool ret);

private slots:
    void on_btnPasswdAuth_clicked();
    void on_btnMore_clicked();
    void on_btnRetry_clicked();

    void onBioAuthNotify(const QString &notifyMsg);
    void onBioAuthComplete(uid_t uid, bool ret);

private:
    void setMovie();
    void setImage();

private:
    Ui::BioAuthWidget *ui;
    BioAuth *bioAuth;
    uid_t uid;
    DeviceInfo device;
};

#endif // BIOAUTHWIDGET_H
