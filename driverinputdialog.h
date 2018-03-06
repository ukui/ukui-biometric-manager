#ifndef DRIVERINPUTDIALOG_H
#define DRIVERINPUTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QRadioButton>

class DriverInputDialog : public QDialog
{
	Q_OBJECT

public:
	DriverInputDialog();

signals:
	void inputCompleted(QString driverName, bool status, QString driverPath, QString devicePath);

private slots:
	void on_btnOK();

private:
	QLineEdit *editDriverName;
	QRadioButton *radioEnable;
	QRadioButton *radioDisable;
	QLineEdit *editDriverPath; /* Shared Library */
	QLineEdit *editDevicePath; /* USB device doesn't have this entry */
};

#endif // DRIVERINPUTDIALOG_H
