#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include "biometric.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    enum Mode{UNDEFINED, PASSWORD, BIOMETRIC, DEVICES};
    void switchWidget(Mode mode);

private:
    Ui::MainWindow *ui;
    BioAuthWidget *widgetBioAuth;
    BioDevicesWidget *widgetBioDevices;
    BioDevices bioDevices;
};

#endif // MAINWINDOW_H
