#ifndef CONFIGURECONSOLESERVERDIALOG_H
#define CONFIGURECONSOLESERVERDIALOG_H

#include <QDialog>
#include <QSettings>
#include "wickrioappsettings.h"

namespace Ui {
class ConfigureConsoleServerDialog;
}

class ConfigureConsoleServerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConfigureConsoleServerDialog(QSettings *settings, WickrIOSSLSettings *sslSettings, QWidget *parent = 0);
    ~ConfigureConsoleServerDialog();

private:
    Ui::ConfigureConsoleServerDialog *ui;
    QSettings *m_settings;
    WickrIOSSLSettings *m_sslSettings;

    bool save();
    void updateDialog();

private slots:
    void slotComboChanged(int);

signals:
    void finished();
};

#endif // CONFIGURECONSOLESERVERDIALOG_H
