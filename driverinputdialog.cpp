#include "driverinputdialog.h"
#include <QGridLayout>
#include <QLabel>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

#define WIDGET_WIDTH 160

DriverInputDialog::DriverInputDialog()
{
	editDriverName = new QLineEdit();
	editDriverName->setFixedWidth(WIDGET_WIDTH);
	editDriverName->setPlaceholderText(tr("Required Field"));
	editDriverPath = new QLineEdit();
	editDriverPath->setFixedWidth(WIDGET_WIDTH);
	editDriverPath->setPlaceholderText(tr("Required Field"));
	editDevicePath = new QLineEdit();
	editDevicePath->setFixedWidth(WIDGET_WIDTH);
	editDevicePath->setPlaceholderText(tr("Optional Field"));

	QVBoxLayout *vBoxLayout = new QVBoxLayout();

	QHBoxLayout *hBoxLayout = new QHBoxLayout();
	hBoxLayout->addWidget(new QLabel(tr("Driver Name")));
	hBoxLayout->addWidget(editDriverName, Qt::AlignRight);
	vBoxLayout->addLayout(hBoxLayout);

	hBoxLayout = new QHBoxLayout();
	hBoxLayout->addWidget(new QLabel(tr("Status")));
	QGroupBox *groupBox = new QGroupBox();
	groupBox->setFixedWidth(WIDGET_WIDTH);
	groupBox->setObjectName("groupBoxDriverInputDialog");
	radioEnable = new QRadioButton(tr("Enable"));
	radioDisable = new QRadioButton(tr("Disable"));
	radioDisable->setChecked(true);
	QHBoxLayout *groupBoxLayout = new QHBoxLayout();
	groupBoxLayout->addWidget(radioEnable);
	groupBoxLayout->addWidget(radioDisable);
	groupBoxLayout->addStretch(1);
	groupBox->setLayout(groupBoxLayout);
	hBoxLayout->addWidget(groupBox, Qt::AlignRight);
	vBoxLayout->addLayout(hBoxLayout);

	hBoxLayout = new QHBoxLayout();
	hBoxLayout->addWidget(new QLabel(tr("Driver Path")));
	hBoxLayout->addWidget(editDriverPath, Qt::AlignRight);
	vBoxLayout->addLayout(hBoxLayout);

	hBoxLayout = new QHBoxLayout();
	hBoxLayout->addWidget(new QLabel(tr("Device Name")));
	hBoxLayout->addWidget(editDevicePath, Qt::AlignRight);
	vBoxLayout->addLayout(hBoxLayout);

	hBoxLayout = new QHBoxLayout();
	QPushButton *btnOK = new QPushButton(tr("OK"));
	QPushButton *btnCancel = new QPushButton(tr("Cancel"));
	btnOK->setFixedSize(60, 26);
	btnCancel->setFixedSize(60, 26);
	hBoxLayout->addWidget(btnCancel, Qt::AlignRight);
	hBoxLayout->addWidget(btnOK, Qt::AlignRight);
	vBoxLayout->addLayout(hBoxLayout);

	vBoxLayout->addSpacerItem(new QSpacerItem(1,1, QSizePolicy::Minimum, QSizePolicy::Expanding));

	this->setLayout(vBoxLayout);

	connect(btnOK, &QPushButton::clicked, this, &DriverInputDialog::on_btnOK);
	connect(btnCancel, &QPushButton::clicked, this, &DriverInputDialog::reject);
}

void DriverInputDialog::on_btnOK()
{
	QString driverName = editDriverName->text();
	bool status = (radioEnable->isChecked()?true:false);
	QString driverPath = editDriverPath->text();
	QString devicePath = editDevicePath->text();

	if (driverName == "") {
		QMessageBox *messageBox = new QMessageBox(QMessageBox::Information,
							tr("Information"),
							tr("Driver Name Can't be Empty"),
							QMessageBox::Ok);
		messageBox->exec();
		return;
	}

	if (driverPath == "") {
		QMessageBox *messageBox = new QMessageBox(QMessageBox::Information,
							tr("Information"),
							tr("Driver Path Can't be Empty"),
							QMessageBox::Ok);
		messageBox->exec();
		return;
	}

	emit inputCompleted(driverName, status, driverPath, devicePath);
	this->accept();
}
