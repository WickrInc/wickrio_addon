#ifndef INITTEST_H
#define INITTEST_H

#include <QObject>
#include <QtTest/QtTest>

class InitTest : public QObject
{
    Q_OBJECT
    public:
        explicit InitTest(QObject *parent = 0);

    private:
        bool isVERSIONDEBUG() { return true; }

    private slots:
        void initTestCase();
        void testRadius();
        void testArea();
        void cleanupTestCase();
};

#endif // INITTEST_H
