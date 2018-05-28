#include "promptdialog.h"
#include "ui_promptdialog.h"
#include <QMovie>
#include <QPushButton>
#include <QDebug>
#include <QCloseEvent>

PromptDialog::PromptDialog(QString gif, QWidget *parent, QString msg) :
	QDialog(parent),
	ui(new Ui::PromptDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose); /* 关闭时自动销毁 */
	QMovie *movie = new QMovie(gif);
	ui->labelImage->setMovie(movie);
	movie->start();
	onlyShowCancel();
	setLabelText(msg);

	/* 设置 CSS */
	QFile qssFile(":/css/assets/promptdialog.qss");
	qssFile.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(qssFile.readAll());
	this->setStyleSheet(styleSheet);
	qssFile.close();
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
    ui->labelText->adjustSize();
}

/**
 * @brief 只显示取消按钮
 */
void PromptDialog::onlyShowCancel()
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
	accept(); /* 用户确定，关闭窗口 */
}

/**
 * @brief 点击取消
 */
void PromptDialog::on_btnCancel_clicked()
{
	/* 使用自定义信号触发主窗口内的函数进行 DBus 相关操作 */
	emit canceled();
}

/**
 * @brief 调用以主动关闭弹窗
 */
void PromptDialog::closeDialog()
{
	accept(); /* 关闭窗口，也可以用 reject()函数 */
}

/**
 * @brief 重写 X 按钮事件，与取消按钮逻辑相同
 * @param event
 */
void PromptDialog::closeEvent(QCloseEvent *event)
{
	qDebug() << "GUI:" << "Click CLOSE";
	emit canceled(); /* 自定义信号 */
	event->ignore(); /* 不再后续处理，否则弹窗会被立刻关闭 */
}
