#ifndef ADVANCEDDIALOG_H
#define ADVANCEDDIALOG_H

#include <QDialog>
#include "wickrioappsettings.h"

namespace Ui {
class AdvancedDialog;
}

class AdvancedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdvancedDialog(WickrIOEmailSettings *emailIN, WickrIOSSLSettings *sslIN, QWidget *parent = 0);
    ~AdvancedDialog();

private:
    void dataToScreen();
    void screenToData();
    void updateScreen();
    void slotComboChanged(int);

public:
    WickrIOEmailSettings *email;
    WickrIOSSLSettings *ssl;
    bool success;
    bool emailSuccess;
    bool sslSuccess;

private:
    Ui::AdvancedDialog *ui;

signals:
    void finished(AdvancedDialog *dlg);
};

#endif // ADVANCEDDIALOG_H
