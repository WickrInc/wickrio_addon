
#include "wickrapplication.h"

#include <QDebug>


WickrApplication* WickrApplication::m_instance = NULL;


WickrApplication::WickrApplication(int &argc, char **argv, bool GUIenabled)
    : QApplication( argc, argv, GUIenabled )
{
    m_instance = this;
}

WickrApplication::WickrApplication(int &argc, char **argv)
    : QApplication( argc, argv )
{
    m_instance = this;
}

WickrApplication::~WickrApplication()
{

}

