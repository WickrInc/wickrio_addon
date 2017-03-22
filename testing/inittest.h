#ifndef INITTEST_H
#define INITTEST_H

#include <QObject>
#include <QtTest/QtTest>

#include "operationdata.h"

class InitTest : public QObject
{
    Q_OBJECT
    public:
        explicit InitTest(int argc, char *argv[], QObject *parent = 0);

    private:
        int     m_argc;
        char    **m_argv;
        OperationData *m_operation;

        bool isVERSIONDEBUG() { return true; }

    private slots:
        void initTestCase();
        void testRadius();
        void testArea();
        void cleanupTestCase();
};

#endif // INITTEST_H
