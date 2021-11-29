#ifndef GRAPHICSITEMZOOMER_H
#define GRAPHICSITEMZOOMER_H

#include <QObject>

class QGraphicsView;

class GraphicsItemZoomer : public QObject
{
    Q_OBJECT
public:
    explicit GraphicsItemZoomer(QGraphicsView *view = nullptr);

    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void zoomRequested(double factor);
};

#endif // GRAPHICSITEMZOOMER_H
