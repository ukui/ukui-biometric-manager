#ifndef INPUTDIALOG_H
#define INPUTDIALOG_H

#include <QDialog>

namespace Ui {
class InputDialog;
}

class InputDialog : public QDialog
{
    Q_OBJECT

public:
    explicit InputDialog(QWidget *parent = 0);
    ~InputDialog();
    void setTitle(const QString &text);
    void setPrompt(const QString &text);
    void setText(const QString &text);
    QString getText();

private slots:
    void on_btnClose_clicked();
    void on_btnOK_clicked();
    void on_lineEdit_returnPressed();

private:
    Ui::InputDialog *ui;
};

#endif // INPUTDIALOG_H
