#include "debugcatcher.h"


#include <QDebug>


namespace {
// код в данном файле позволяет отлавливать все сообщения стандартного логгера Qt (например, qDebug() << ...)
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg);

// извлечь стандартный и установить новый
QtMessageHandler standardLogger{ qInstallMessageHandler(myMessageOutput) };

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // использование стандартной реализации
    standardLogger(type, context, msg);

    if (type == QtMsgType::QtInfoMsg)
        DebugCatcher::instance()->catchInfoMessage(msg);
    else if (type == QtMsgType::QtWarningMsg)
        DebugCatcher::instance()->catchWarningMessage(msg);
}
} // end anonymous namespace


DebugCatcher *DebugCatcher::instance()
{
    static DebugCatcher dc;
    return &dc;
}

void DebugCatcher::catchInfoMessage(const QString &string)
{
    emit messageRecieved(string);
}

void DebugCatcher::catchWarningMessage(const QString &string)
{
    ++infoWarnings;
    emit messageRecieved(string);
}
