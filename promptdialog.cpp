#include "promptdialog.h"
#include "ui_promptdialog.h"
#include <QMovie>
#include <QPushButton>
#include <QDebug>

PromptDialog::PromptDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::PromptDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose); /* 关闭时自动销毁 */
	QMovie *movie = new QMovie(":/images/assets/progressbar.gif");
	ui->labelImage->setMovie(movie);
	movie->start();
	onlyShowCancle();
	setLabelText("操作中，请稍后...");
}

PromptDialog::~PromptDialog()
{
	delete ui;
}

/**
 * @brief 设置要显示的信息
 * @param text
 */
void PromptDialog::setLabelText(QString text)
{
	ui->labelText->setText(text);
}

/**
 * @brief 只显示取消按钮
 */
void PromptDialog::onlyShowCancle()
{
	ui->btnCancel->show();
	ui->btnOK->hide();
}

/**
 * @brief 只显示确定按钮
 */
void PromptDialog::onlyShowOK()
{
	ui->btnOK->show();
	ui->btnCancel->hide();
}

/**
 * @brief 点击确定
 */
void PromptDialog::on_btnOK_clicked()
{
	accept();
}

/**
 * @brief 点击取消
 */
void PromptDialog::on_btnCancel_clicked()
{
	emit canceled();
}

/**
 * @brief 关闭弹窗
 */
void PromptDialog::closeDialog()
{
	accept();
}

void PromptDialog::closeEvent(QCloseEvent *e)
{
	qDebug() << "GUI:" << "Click CLOSE";
	emit canceled();
}
