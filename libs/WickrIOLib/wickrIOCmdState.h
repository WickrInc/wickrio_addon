#ifndef WICKRIOCMDSTATE_H
#define WICKRIOCMDSTATE_H

#include <QObject>
#include <QString>

class WickrIOCmdState {

public:
    WickrIOCmdState(const QString key) : m_key(key) {}
    WickrIOCmdState(const QString key, const QString originalCommand) : m_key(key), m_originalCommand(originalCommand) {}

    // setters
    void setOriginalCommand(const QString originalCommand) { m_originalCommand = originalCommand; }
    void setCommand(int command) { m_command = command; }
    void setState(int state) { m_state = state; }
    void setInput(const QString input) { m_input = input; }
    void setOutput(const QString output) { m_output = output; }
    void setDone() { m_done = true; }

    // getters
    QString originalCommand() { return m_originalCommand; }
    int command() { return m_command; }
    int state() { return m_state; }
    const QString input() { return m_input; }
    const QString output() { return m_output; }
    bool done() { return m_done; }

private:
    QString m_key;
    QString m_originalCommand;  // The text of the original command

    int     m_command = 0;      // Which command (sequence) is being processed
    int     m_state = 0;        // current state
    bool    m_done = false;     // Set to true when done
    QString m_input;            // input value from the client
    QString m_output;           // output value to send to the client

};


#endif // WICKRIOCMDSTATE_H
