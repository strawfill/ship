#ifndef DEBUGCATCHER_H
#define DEBUGCATCHER_H

#include <QObject>

class DebugCatcher : public QObject
{
    Q_OBJECT
public:
    static DebugCatcher *instance();

    int warningsCount() const { return infoWarnings; }
    void clearWaringsCount() { infoWarnings = 0; }

    void catchInfoMessage(const QString &string);
    void catchWarningMessage(const QString &string);

signals:
    void messageRecieved(const QString &string);

private:
    DebugCatcher() =default;

private:
    int infoWarnings{ 0 };
};

#endif // DEBUGCATCHER_H
