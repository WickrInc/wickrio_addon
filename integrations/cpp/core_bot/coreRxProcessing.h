#ifndef CORERXPROCESSING_H
#define CORERXPROCESSING_H

#include <QObject>
#include "coreOperationData.h"

class CoreRxProcessing : public QObject
{
    Q_OBJECT
public:
    CoreRxProcessing(CoreOperationData *operation, QObject *parent = nullptr);
    ~CoreRxProcessing();

    bool timerCall();

private:
    CoreOperationData   *m_operation;

signals:

public slots:
};

#endif // CORERXPROCESSING_H
