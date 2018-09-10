/*
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * 
**/
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
