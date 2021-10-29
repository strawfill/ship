#include "ignoredndgraphicsview.h"

#include <QDragMoveEvent>
#include <QMimeData>

void IgnoreDndGraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
    if (needIgnore(event))
        return QWidget::dragEnterEvent(event);
    return QGraphicsView::dragEnterEvent(event);
}

void IgnoreDndGraphicsView::dragMoveEvent(QDragMoveEvent *event)
{
    if (needIgnore(event))
        return QWidget::dragMoveEvent(event);
    return QGraphicsView::dragMoveEvent(event);
}

void IgnoreDndGraphicsView::dropEvent(QDropEvent *event)
{
    if (needIgnore(event))
        return QWidget::dropEvent(event);
    return QGraphicsView::dropEvent(event);
}

bool IgnoreDndGraphicsView::needIgnore(QDropEvent *event)
{
    if (event->mimeData()) {
        const auto & m{ event->mimeData() };
        if (m->hasText() || m->hasUrls())
            return true;
    }
    return false;
}
