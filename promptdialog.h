#ifndef PROMPTDIALOG_H
#define PROMPTDIALOG_H

#include <QDialog>

namespace Ui {
class PromptDialog;
}

class PromptDialog : public QDialog
{
	Q_OBJECT

public:
	explicit PromptDialog(QWidget *parent = 0);
	~PromptDialog();

signals:
	void canceled();

public:
	void setLabelText(QString text);
	void onlyShowCancle();
	void onlyShowOK();
	void closeDialog();

private slots:
	void on_btnOK_clicked();

	void on_btnCancel_clicked();

private:
	Ui::PromptDialog *ui;

protected:
	void closeEvent(QCloseEvent *e);
};

#endif // PROMPTDIALOG_H
