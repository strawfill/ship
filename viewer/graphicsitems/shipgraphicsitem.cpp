#include "shipgraphicsitem.h"

#include <valarray>
#include <QtMath>

using namespace prepared;

namespace {
constexpr int initRotation{ 0 };
}

ShipGraphicsItem::ShipGraphicsItem(const QPixmap &pixmap, int aspeed, const prepared::Path &apath)
    : QGraphicsPixmapItem(pixmap)
    , path(apath)
    , offset(pixmap.width()/2, pixmap.height()/2)
    , speed(aspeed)
{
    setShipPosition(0, 0);
    setRotation(initRotation);
    setTransformOriginPoint(offset);
}

void ShipGraphicsItem::setHour(double hour)
{
    if (qFuzzyIsNull(hour)) {
        setShipPosition(0, 0);
        setRotation(initRotation);
        return;
    }

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
            rotationAtTracStart = qRound(rotation());
            return {path.at(memoriesIndex), path.at(memoriesIndex+1)};
        }
    }

    if (path.last().timeH < hour) {
        memoriesIndex = path.size()-2;
        rotationAtTracStart = qRound(rotation());
        return {path.at(memoriesIndex), path.at(memoriesIndex+1)};
    }

    // надеюсь, такого исхода нет
    return {};
}

void ShipGraphicsItem::setShipPosition(double x, double y)
{
    // не понимаю, почему здесь смещение должно быть по оси y отрицательным
    // очень многое сделано на подборе
    QPointF current{ QPointF{ x, y } - QPoint{ offset.x(), -offset.y() }  };
    if (pos() != current) {
        // система координат конечно немного не та...
        // а делать scale(1, -1) на view - тоже не помогает особо, ведь сторонние эффекты всё портят
        setPos(current.x(), -current.y());
        update();
    }
}

void ShipGraphicsItem::setShipPosition(double x1, double y1, double x2, double y2, int minH, int maxH, double curH)
{
    if (minH == maxH)
        return setShipPosition(x2, y2);

    // закомментирована реализация, которая ничего не знала о скорости судна и выравнивала его относительно общего времени
    // это искажало реальную скорость и делало симуляцию менее наглядной и соответствующей реальности
    //double percent{ qBound(0., (curH - minH) / (maxH-minH), 1.) };
    double dx{ (x2-x1) };
    double dy{ (y2-y1) };
    double percent{ qBound(0., (curH - minH) * speed / qSqrt(dx*dx + dy*dy), 1.) };
    dx *= percent;
    dy *= percent;

    setShipPosition(x1+dx, y1+dy);

    // а теперь хотелось бы, что бы корабль мог нормально крутиться

    setShipRotation(std::atan2(y1-y2, x2-x1) * 180 / M_PI, curH - minH);
}

void ShipGraphicsItem::setShipRotation(double rotation, double hourInThatTrac)
{
    rotation += 90;

    int inewrot{ int(rotation) };
    inewrot %= 360;

    int icurrot{ int(this->rotation()) };

    if (inewrot != icurrot) {
        if (hourInThatTrac > 1.) {
            setRotation(inewrot);
            update();
            return;
        }

        int delta{ inewrot - rotationAtTracStart };
        if (qAbs(delta) < 180) {
            // по часовой крутится, всё хорошо
            double resrot{ rotationAtTracStart + delta * qBound(0., hourInThatTrac, 1.) };
            setRotation(qRound(resrot));
        }
        else {
            // методом подбора вся реализация функции и построена
            // не до конца понимаю, почему именно такие числа нужно вставлять
            delta = delta - 360;
            double resrot{ rotationAtTracStart + delta * qBound(0., hourInThatTrac, 1.) };
            setRotation(qRound(resrot));
        }

        //int maxdelta = 7;
        //int fulldelta = icurrot - ibefrot;
        //if (qAbs(fulldelta) > 180)
        //    fulldelta = -fulldelta;
        //int delta = qBound(-maxdelta, fulldelta, maxdelta);
        //
        //int result = (icurrot + delta) % 360;
        //
        //setRotation(result);
        update();
    }
}
