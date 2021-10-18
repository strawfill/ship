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
}
} // end anonymous namespace


DebugCatcher *DebugCatcher::instance()
{
    static DebugCatcher dc;
    return &dc;
}

void DebugCatcher::catchInfoMessage(const QString &string)
{
    ++infoWarnings;
    emit infoMessageRecieved(string);
}
