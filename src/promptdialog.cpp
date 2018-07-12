#include "promptdialog.h"
#include "ui_promptdialog.h"
#include <QMovie>
#include <QPushButton>
#include <QDebug>
#include <QCloseEvent>
#include <QStandardItemModel>
#include <pwd.h>

PromptDialog::PromptDialog(QString gif, QWidget *parent, QString msg)
    : QDialog(parent),
      ui(new Ui::PromptDialog)
{
	ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose); /* 关闭时自动销毁 */
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    ui->btnClose->setFlat(true);
    ui->btnClose->setIcon(QIcon(":/images/assets/close.png"));

	QMovie *movie = new QMovie(gif);
    ui->lblImage->setMovie(movie);
    movie->start();
	setLabelText(msg);

	/* 设置 CSS */
	QFile qssFile(":/css/assets/promptdialog.qss");
	qssFile.open(QFile::ReadOnly);
	QString styleSheet = QLatin1String(qssFile.readAll());
	this->setStyleSheet(styleSheet);
	qssFile.close();
    ui->treeViewResult->hide();
}

PromptDialog::~PromptDialog()
{
	delete ui;
}

void PromptDialog::setTitle(const QString &title)
{
    ui->lblTitle->setText(title);
}

/**
 * @brief 设置要显示的信息
 * @param text
 */
void PromptDialog::setLabelText(QString text)
{
    ui->lblPrompt->setText(text);
    ui->lblPrompt->setWordWrap(true);
    ui->lblPrompt->adjustSize();
}

void PromptDialog::on_btnClose_clicked()
{
    close();
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

void PromptDialog::setSearchResult(bool isAdmin, const QList<SearchResult> &searchResultList)
{
    QStandardItemModel *model = new QStandardItemModel(ui->treeViewResult);
    if(isAdmin)
        model->setHorizontalHeaderLabels(QStringList{"    " + tr("Index"), tr("UserName"), tr("FeatureName")});
    else
        model->setHorizontalHeaderLabels(QStringList{"    " + tr("Index"), tr("FeatureName")});

    for(int i = 0; i < searchResultList.size(); i++) {
        SearchResult ret = searchResultList[i];
        QList<QStandardItem*> row;
        row.append(new QStandardItem(QString::number(i+1)));
        if(isAdmin) {
            struct passwd *pwd = getpwuid(ret.uid);
            row.append(new QStandardItem(QString(pwd->pw_name)));
        }
        row.append(new QStandardItem(ret.indexName));
        model->appendRow(row);
    }

    ui->treeViewResult->setModel(model);
    ui->treeViewResult->show();
    this->setFixedHeight(height() + 100);
}
