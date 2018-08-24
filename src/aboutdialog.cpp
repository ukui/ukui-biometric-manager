#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QFile>
#include <QDebug>

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);

    QFile qssFile(":/css/assets/promptdialog.qss");
    qssFile.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(qssFile.readAll());
    this->setStyleSheet(styleSheet);
    qssFile.close();

    ui->btnClose->setIcon(QIcon(":/images/assets/close.png"));

    connect(ui->btnClose, &QPushButton::clicked, this, &AboutDialog::close);
    connect(ui->btnCloseDialog, &QPushButton::clicked, this, &AboutDialog::close);
    connect(ui->btnAbout, &QPushButton::clicked, this, [&]{
        ui->stackedWidget->setCurrentWidget(ui->pageAbout);
        ui->lblIndAbout->setStyleSheet("QLabel{background:url(:/images/assets/underline.png)}");
        ui->lblIndContrib->setStyleSheet("QLabel{background:transparent;}");
    });
    connect(ui->btnContributor, &QPushButton::clicked, this, [&]{
        ui->stackedWidget->setCurrentWidget(ui->pageContributor);
        ui->lblIndContrib->setStyleSheet("QLabel{background:url(:/images/assets/underline.png)}");
        ui->lblIndAbout->setStyleSheet("QLabel{background:transparent;}");
    });
    ui->btnAbout->click();

    QString locale = QLocale::system().name();
    if(locale == "zh_CN")
        setFixedHeight(280);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
