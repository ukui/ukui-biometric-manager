#ifndef MESSAGEDIALOG_H
#define MESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
class MessageDialog;
}

class MessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MessageDialog(int type = Normal, const QString &title = "",
                           const QString &msg = "", QWidget *parent = 0);
    ~MessageDialog();
    void setTitle(const QString &text);
    void setMessage(const QString &text);
    void setMessageList(const QStringList &textList);
    void setOkText(const QString &text);
    void setCancelText(const QString &text);
    enum{Normal, Error, Question};

private:
    Ui::MessageDialog *ui;
};

#endif // MESSAGEDIALOG_H
