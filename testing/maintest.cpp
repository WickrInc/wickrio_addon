#include <QtTest/QtTest>
#include "inittest.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(testing);

    InitTest ct(argc,argv);
    QTest::qExec(&ct);

    return 0;
}
