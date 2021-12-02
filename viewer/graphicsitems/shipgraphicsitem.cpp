#include "shipgraphicsitem.h"

#include <valarray>
#include <QtMath>
#include <QPainter>

using namespace prepared;

namespace {
constexpr int initRotation{ 0 };
constexpr QSize size{ 8, 26 };
}

ShipGraphicsItem::ShipGraphicsItem(raw::Ship::Type atype, int aspeed, const prepared::Path &apath, double amodifier)
    : QGraphicsPathItem(atype == raw::Ship::Type::handler ? handlerPath() : shooterPath())
    , mpath(apath)
    , speed(aspeed)
    , modifier(amodifier)
    , offset(size.width()/2, size.height()/2)
    , type(atype)
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

    if (mpath.size() < 2)
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
    if (mpath.size() < 2)
        return {};

    // попытаемся воспользоваться запомненным индексом, чтобы долго не искать две точки

    // если запомненный индекс полностью подходит
    if (memoriesIndex >= 0 && memoriesIndex+1 < mpath.size() &&
            mpath.at(memoriesIndex).timeH <= hour && mpath.at(memoriesIndex+1).timeH >= hour) {
        return {mpath.at(memoriesIndex), mpath.at(memoriesIndex+1)};
    }

    // если запомненный индекс некорректен
    if (memoriesIndex < 0 || memoriesIndex+1 >= mpath.size() || mpath.at(memoriesIndex).timeH > hour) {
        // сбросим его
        memoriesIndex = 0;
    }

    //теперь будем перебирать по точкам...
    for (int i = 1; i < mpath.size(); ++i) {
        if (mpath.at(i-1).timeH <= hour && mpath.at(i).timeH >= hour) {
            memoriesIndex = i-1;
            rotationAtTracStart = qRound(rotation());
            return {mpath.at(memoriesIndex), mpath.at(memoriesIndex+1)};
        }
    }

    if (mpath.last().timeH < hour) {
        memoriesIndex = mpath.size()-2;
        rotationAtTracStart = qRound(rotation());
        return {mpath.at(memoriesIndex), mpath.at(memoriesIndex+1)};
    }

    // надеюсь, такого исхода нет
    return {};
}

void ShipGraphicsItem::setShipPosition(double x, double y)
{
    // не понимаю, почему здесь смещение должно быть по оси y отрицательным
    // очень многое сделано на подборе
    QPointF current{ x, y };
    current *=  modifier;
    current -= QPointF{ offset.x(), -offset.y() };
    if (pos() != current) {
        // система координат конечно немного не та...
        // а делать scale(1, -1) на view - тоже не помогает особо, ведь сторонние эффекты всё портят
        prepareGeometryChange();
        setPos(current.x(), -current.y());
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
    double part{ qBound(0., (curH - minH) * speed / qSqrt(dx*dx + dy*dy), 1.) };
    dx *= part;
    dy *= part;

    setShipPosition(x1+dx, y1+dy);

    // а теперь хотелось бы, что бы корабль мог нормально крутиться
    setShipRotation(-std::atan2(y2-y1, x2-x1) * 180 / M_PI, curH - minH, part);
}

void ShipGraphicsItem::setShipRotation(double rotation, double hourInThatTrac, double tracPart)
{
    // реализация такая, что корабль полностью плавно повернётся на новое направление по минимальному условию из
    //  а) в течение rotationTime часа движения по текущему треку
    constexpr double rotationTime{ 0.6 };
    //  б) в течение rotationTracPart части всего пути по текущему треку
    constexpr double rotationTracPart{ 0.3 };

    // какая-то фигня, подбором поставлена
    rotation += 90;

    int inewrot{ int(rotation) };
    inewrot %= 360;

    int icurrot{ int(this->rotation()) };


    if (inewrot != icurrot) {
        if (hourInThatTrac > rotationTime || tracPart > rotationTracPart) {
            prepareGeometryChange();
            setRotation(inewrot);
            return;
        }

        int delta{ (inewrot - rotationAtTracStart) % 360 };
        // и теперь 2 небольшие правки, чтобы крутилось по минимальному повороту
        if (delta < -180)
            delta += 360;
        if (delta > 180)
            delta -= 360;

        const double condition1{ hourInThatTrac / rotationTime };
        const double condition2{ tracPart / rotationTracPart };

        double resrot{ rotationAtTracStart + delta * qBound(0., qMax(condition1, condition2), 1.) };
        prepareGeometryChange();
        setRotation(qRound(resrot));
    }
}

QPainterPath ShipGraphicsItem::handlerPath()
{
    const int height{ size.height() };
    int partHair{ 4 }; // высота начального закругления
    int partTail{ 2 }; // высота конечной кабины
    QRect beginRect{ QPoint{1,0}, QSize{ size.width()-2, partHair*2 } };
    QRect bodyRect{ QPoint{1,partHair}, QSize{ size.width()-1, height - partHair - partTail } };

    QPainterPath pp;
    pp.arcMoveTo(beginRect, 0);
    pp.arcTo(beginRect, 0, 180);
    pp.lineTo(bodyRect.topLeft());
    pp.lineTo(bodyRect.bottomLeft());
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{-1, 0});
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{-1, 1});
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{ 1, 1});
    pp.lineTo(bodyRect.bottomRight() += QPoint{-1, 1});
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{ 1, 1});
    int deltaToBottom{ height - bodyRect.bottom() };
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{ 1, deltaToBottom});
    pp.lineTo(bodyRect.bottomRight() += QPoint{-1, deltaToBottom});
    pp.lineTo(bodyRect.bottomRight() += QPoint{-1, 1});
    pp.lineTo(bodyRect.bottomRight() += QPoint{ 1, 1});
    pp.lineTo(bodyRect.bottomRight() += QPoint{ 1, 0});

    pp.lineTo(bodyRect.bottomRight());
    pp.lineTo(bodyRect.topRight());

    for (int i = bodyRect.top(); i < bodyRect.bottom()-1; i += 3) {
        pp.addEllipse(bodyRect.left(), i, 1, 1);
        pp.addEllipse(bodyRect.right()-1, i, 1, 1);
    }

    pp.moveTo(QPoint{size.width()/2, partHair/3});
    pp.lineTo(QPoint{size.width()/2, bodyRect.bottom()-3});

    return pp;
}

QPainterPath ShipGraphicsItem::shooterPath()
{
    const int height{ size.height() };
    int partHair{ 16 }; // высота начального закругления
    int partTail{  4 }; // высота конечной кабины
    QRect beginRect{ QPoint{1,0}, QSize{ size.width()-2, partHair*2 } };
    QRect bodyRect{ QPoint{1,partHair}, QSize{ size.width()-1, height - partHair - partTail } };

    QPainterPath pp;
    pp.arcMoveTo(beginRect, 0);
    pp.arcTo(beginRect, 0, 180);
    pp.lineTo(bodyRect.topLeft());
    pp.lineTo(bodyRect.bottomLeft());
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{-1, 0});
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{-1, 1});
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{ 1, 1});
    pp.lineTo(bodyRect.bottomRight() += QPoint{-1, 1});
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{ 1, 1});
    int deltaToBottom{ height - bodyRect.bottom() };
    pp.lineTo(bodyRect.bottomLeft()  += QPoint{ 1, deltaToBottom});
    pp.lineTo(bodyRect.bottomRight() += QPoint{-1, deltaToBottom});
    pp.lineTo(bodyRect.bottomRight() += QPoint{-1, 1});
    pp.lineTo(bodyRect.bottomRight() += QPoint{ 1, 1});
    pp.lineTo(bodyRect.bottomRight() += QPoint{ 1, 0});

    pp.lineTo(bodyRect.bottomRight());
    pp.lineTo(bodyRect.topRight());

    pp.moveTo(QPoint{size.width()/2, partHair/3});
    pp.lineTo(QPoint{size.width()/2+1, bodyRect.bottom()-3});
    pp.lineTo(QPoint{size.width()/2-1, bodyRect.bottom()-3});
    pp.lineTo(QPoint{size.width()/2, partHair/3});

    return pp;
}


void ShipGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // переопределён с целью добавления градиента
    Q_UNUSED(widget)
    Q_UNUSED(option)
    painter->setPen(pen());
    QLinearGradient gradient(0, 0, 0, size.height());
    if (type == raw::Ship::Type::handler) {
        gradient.setColorAt(0.0, QColor{0xf0, 0xf0, 0x40});
        gradient.setColorAt(1.0, QColor{0xcf, 0x33, 0x00});
    }
    else {
        gradient.setColorAt(0.0, QColor{0x40, 0x40, 0xf0});
        gradient.setColorAt(1.0, QColor{0x22, 0xff, 0x22});
    }
    painter->setBrush(gradient);
    painter->drawPath(path());

    // выделение не поддерживаем, поэтому не будем что-то делать с этим
}
