#ifndef ADDCONSOLEUSERDIALOG_H
#define ADDCONSOLEUSERDIALOG_H

#include <QDialog>

#include "wickriodatabase.h"

namespace Ui {
class AddConsoleUserDialog;
}

class AddConsoleUserDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddConsoleUserDialog(WickrIOClientDatabase *ioDB, WickrIOConsoleUser *consoleUser, QWidget *parent = 0);
    ~AddConsoleUserDialog();

private:
    WickrIOClientDatabase *m_ioDB;
    WickrIOConsoleUser *m_consoleUser;

    Ui::AddConsoleUserDialog *ui;

    void updateButtons();
    bool saveScreenValues();

signals:
    void finished();

private slots:
    void slotComboChanged(int);

};

#endif // ADDCONSOLEUSERDIALOG_H
