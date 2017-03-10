#ifndef WICKRAPPLICATION
#define WICKRAPPLICATION

#ifdef Q_OS_MAC
#include <objc/objc.h>
#include <objc/message.h>
#endif

#include <QApplication>

class WickrApplication : public QApplication
{
    Q_OBJECT
    Q_DISABLE_COPY(WickrApplication)

public:
    explicit WickrApplication(int &argc, char **argv, bool GUIenabled = true);
    explicit WickrApplication(int &argc, char **argv);
    virtual ~WickrApplication();

private:
    static WickrApplication* m_instance;

};

#endif // WICKRAPPLICATION

