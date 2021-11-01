#ifndef SHIPGRAPHICSITEM_H
#define SHIPGRAPHICSITEM_H

#include <QGraphicsPathItem>
#include "graphicsiteminterface.h"

#include "prepareddata.h"

class ShipGraphicsItem : public QGraphicsPathItem, public GraphicsItemInterface
{
public:
    ShipGraphicsItem(raw::Ship::Type atype, int aspeed, const prepared::Path &apath);

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    void setHour(double hour) override;

private:
    QPair<prepared::PathDot, prepared::PathDot> getCurrentDots(double hour) const;
    void setShipPosition(double x, double y);
    void setShipPosition(double x1, double y1, double x2, double y2, int minH, int maxH, double curH);
    void setShipRotation(double rotation, double hourInThatTrac);

    static QPainterPath handlerPath();
    static QPainterPath shooterPath();

private:
    prepared::Path mpath;
    raw::Ship::Type type;
    QPoint offset;
    int speed{};
    mutable int rotationAtTracStart{};
    mutable int memoriesIndex{};
};

#endif // SHIPGRAPHICSITEM_H
