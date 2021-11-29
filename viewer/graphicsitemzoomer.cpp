#include "graphicsitemzoomer.h"

#include <QEvent>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QtMath>

GraphicsItemZoomer::GraphicsItemZoomer(QGraphicsView *view) : QObject(view)
{
    view->viewport()->installEventFilter(this);
}


bool GraphicsItemZoomer::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched)
    if (event->type() != QEvent::Wheel)
        return false;

    auto wheelEvent = static_cast<QWheelEvent*>(event);

    if (wheelEvent->buttons() != Qt::NoButton || wheelEvent->modifiers() != (Qt::ShiftModifier|Qt::ControlModifier))
        return false;

    if (wheelEvent->orientation() != Qt::Vertical)
        return false;

    double angle = wheelEvent->angleDelta().y();
    double factor = qPow(1.0015, angle);

    emit zoomRequested(factor);
    return true;
}
