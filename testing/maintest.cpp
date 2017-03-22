#include <QtTest/QtTest>
#include "inittest.h"

int main(int argc, char *argv[])
{
    InitTest ct(argc,argv);
    QTest::qExec(&ct);

    return 0;
}
