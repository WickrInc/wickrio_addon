#ifndef WELCOMERXPROCESSING_H
#define WELCOMERXPROCESSING_H

#include <QObject>
#include "parseroperationdata.h"

class WelcomeRxProcessing : public QObject
{
    Q_OBJECT
public:
    WelcomeRxProcessing(ParserOperationData *operation, QObject *parent = nullptr);
    ~WelcomeRxProcessing();

    bool timerCall();

private:
    ParserOperationData *m_operation;

signals:

public slots:
};

#endif // WELCOMERXPROCESSING_H
