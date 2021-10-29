#ifndef SHIPGRAPHICSITEM_H
#define SHIPGRAPHICSITEM_H

#include <QGraphicsPixmapItem>
#include "graphicsiteminterface.h"

#include "prepareddata.h"

class ShipGraphicsItem : public QGraphicsPixmapItem, public GraphicsItemInterface
{
public:
    ShipGraphicsItem(const QPixmap &pixmap, int aspeed, const prepared::Path &apath);

    void setHour(double hour) override;

private:
    QPair<prepared::PathDot, prepared::PathDot> getCurrentDots(double hour) const;
    void setShipPosition(double x, double y);
    void setShipPosition(double x1, double y1, double x2, double y2, int minH, int maxH, double curH);
    void setShipRotation(double rotation);

private:
    prepared::Path path;
    QPoint offset;
    int speed{};
    mutable int memoriesIndex{};
};

#endif // SHIPGRAPHICSITEM_H
