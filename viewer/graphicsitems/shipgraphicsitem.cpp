#include "shipgraphicsitem.h"

#include <valarray>

using namespace prepared;

ShipGraphicsItem::ShipGraphicsItem(const QPixmap &pixmap, int aspeed, const prepared::Path &apath)
    : QGraphicsPixmapItem(pixmap)
    , path(apath)
    , offset(-pixmap.width()/2, -pixmap.height()/2)
    , speed(aspeed)
{
    setShipPosition(0, 0);
    setTransformOriginPoint(-offset);
}

void ShipGraphicsItem::setHour(double hour)
{
    if (path.size() < 2)
        return;

    auto dots{ getCurrentDots(hour) };
    auto f{ dots.first };
    auto s{ dots.second };

    // значит, мы уже на последнем действии
    if (s.timeH <= hour) {
        setShipPosition(s.x, s.y);
    }

    if (f.timeH <= hour && s.timeH >= hour) {
        if (f.activity == sa_waiting) {
            setShipPosition(f.x, f.y);
        }
        else {
            setShipPosition(f.x, f.y, s.x, s.y, f.timeH, s.timeH, hour);
        }
    }

}

QPair<prepared::PathDot, prepared::PathDot> ShipGraphicsItem::getCurrentDots(double hour) const
{
    if (path.size() < 2)
        return {};

    // попытаемся воспользоваться запомненным индексом, чтобы долго не искать две точки

    // если запомненный индекс полностью подходит
    if (memoriesIndex >= 0 && memoriesIndex+1 < path.size() &&
            path.at(memoriesIndex).timeH <= hour && path.at(memoriesIndex+1).timeH >= hour) {
        return {path.at(memoriesIndex), path.at(memoriesIndex+1)};
    }

    // если запомненный индекс некорректен
    if (memoriesIndex < 0 || memoriesIndex+1 >= path.size() || path.at(memoriesIndex).timeH > hour) {
        // сбросим его
        memoriesIndex = 0;
    }

    //теперь будем перебирать по точкам...
    for (int i = 1; i < path.size(); ++i) {
        if (path.at(i-1).timeH <= hour && path.at(i).timeH >= hour) {
            memoriesIndex = i-1;
            return {path.at(memoriesIndex), path.at(memoriesIndex+1)};
        }
    }

    if (path.last().timeH < hour) {
        memoriesIndex = path.size()-2;
        return {path.at(memoriesIndex), path.at(memoriesIndex+1)};
    }

    // надеюсь, такого исхода нет
    return {};
}

void ShipGraphicsItem::setShipPosition(double x, double y)
{
    QPointF current{ QPointF{ x, y } + offset };
    if (pos() != current) {
        setPos(current);
        update();
    }
}

void ShipGraphicsItem::setShipPosition(double x1, double y1, double x2, double y2, int minH, int maxH, double curH)
{
    if (minH == maxH)
        return setShipPosition(x2, y2);

    double percent{ qBound(0., (curH - minH) / (maxH-minH), 100.) };
    double dx{ (x2-x1)*percent };
    double dy{ (y2-y1)*percent };

    setShipPosition(x1+dx, y1+dy);

    // а теперь хотелось бы, что бы корабль мог нормально крутиться

    setShipRotation(std::atan2(y2-y1, x2-x1) * 180 / M_PI);
}

void ShipGraphicsItem::setShipRotation(double rotation)
{
    rotation += 90;

    int icurrot{ int(rotation) };
    icurrot %= 360;

    int ibefrot{ int(this->rotation()) };

    if (icurrot != ibefrot) {
        int maxdelta = 7;
        int fulldelta = icurrot - ibefrot;
        if (qAbs(fulldelta) > 180)
            fulldelta = -fulldelta;
        int delta = qBound(-maxdelta, fulldelta, maxdelta);

        int result = (ibefrot + delta) % 360;

        setRotation(result);
        update();
    }
}
