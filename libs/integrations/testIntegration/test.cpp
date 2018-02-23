#include <QDebug>
#include "test.h"

QString processInput(QString input)
{
    qDebug() << "Received " << input;
    return QString("received your request");
}
