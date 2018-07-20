#ifndef PROMPTDIALOG_H
#define PROMPTDIALOG_H

#include "customtype.h"

#include <QDialog>

namespace Ui {
class PromptDialog;
}

class PromptDialog : public QDialog
{
	Q_OBJECT

public:
    explicit PromptDialog(QDBusInterface *service, int bioType,
                          int deviceId, int uid, QWidget *parent = 0);
	~PromptDialog();
    enum Result {SUCESS, ERROR, UNDEFINED};

public:
    void setTitle(int opsType);
    void setPrompt(const QString &text);

    int enroll(int drvId, int uid, int idx, const QString &idxName);
    int verify(int drvId, int uid, int idx);
    int search(int drvId, int uid, int idxStart, int idxEnd);
    Result getResult();

protected:
    void keyPressEvent(QKeyEvent *);

private:
    void setFailed();
    QString getGif(int type);
    QString getImage(int type);
    void handleErrorResult(int error);
    void showClosePrompt();
    void setSearchResult(bool isAdmin, const QList<SearchResult> &searchResultList);

signals:
    void canceled(); /* 自定义信号，用于触发主窗口内的 slot */

private slots:
    void on_btnClose_clicked();
    void onStatusChanged(int, int);
    void enrollCallBack(const QDBusMessage &);
    void verifyCallBack(const QDBusMessage &);
    void searchCallBack(const QDBusMessage &);
    void StopOpsCallBack(const QDBusMessage &);
    void errorCallBack(const QDBusError &);

private:
	Ui::PromptDialog *ui;
    QDBusInterface *serviceInterface;
    QMovie *movie;
    int type;
    int deviceId;
    int uid;
    bool isEnrolling;
    enum OPS{IDLE, ENROLL, VERIFY, SEARCH} ops;
    Result opsResult;
};

#endif // PROMPTDIALOG_H
