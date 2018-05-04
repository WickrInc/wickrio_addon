#include <QCoreApplication>
#include <QObject>
#include <QTextStream>
#include <QTimer>
#include <QDebug>

#include "test_api_queue.h"

QStringList messagesToSend;
TestAPIQueue *client;
QCoreApplication *app = NULL;

int main(int argc, char **argv)
{
    app = new QCoreApplication(argc, argv);

    client = new TestAPIQueue();
    if (!client->connectToServer())
        return EXIT_FAILURE;

    QObject::connect(client, &TestAPIQueue::signalReady, [=]() {
        QTextStream s(stdin);
        QString lineInput;

        while (true) {
            qDebug() << "Enter command:";
            QString newValue;
            lineInput = s.readLine();
            newValue = lineInput.trimmed();
            if (newValue.toLower() == "quit") {
                app->quit();
                break;
            }

            client->call(newValue);

            {
                QTimer timer;
                QEventLoop loop;

                loop.connect(client, SIGNAL(signalGotResponse()), SLOT(quit()));
                QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

                int loopCount = 6;

                while (loopCount-- > 0) {
                    timer.start(10000);
                    loop.exec();

                    if (timer.isActive()) {
                        timer.stop();
                        QString response = client->getResponse();
                        qDebug().noquote().nospace() << "Reponse: " << response;
                        break;
                    } else {
                        qDebug() << "Timed out waiting for response!";
                        break;
                    }
                }
            }

        }
    });
    client->start();
}
