#include <QtTest/QtTest>
#include "inittest.h"

int main()
{
    InitTest ct;
    QTest::qExec(&ct);

    return 0;
}
