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
	explicit PromptDialog(QString gif,
		QWidget *parent = 0,
		QString msg = tr("Operations are in progress. Please wait..."));
	~PromptDialog();

signals:
	void canceled(); /* 自定义信号，用于触发主窗口内的 slot */

public:
	void setLabelText(QString text);
	void onlyShowCancel();
	void onlyShowOK();
	void closeDialog();

private slots:
	void on_btnOK_clicked();

	void on_btnCancel_clicked();

private:
	Ui::PromptDialog *ui;

protected:
	void closeEvent(QCloseEvent *event);
};

#endif // PROMPTDIALOG_H
