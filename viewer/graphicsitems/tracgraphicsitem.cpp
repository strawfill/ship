#include "tracgraphicsitem.h"

#include <QPen>
#include <QtMath>

TracGraphicsItem::TracGraphicsItem(const prepared::Trac &atrac)
    : QGraphicsLineItem(atrac.line())
    , trac(atrac)
{
    auto p{ pen() };
    p.setWidth(3);
    setPen(p);

    setTracEnabled(false);
}


void TracGraphicsItem::setHour(double hour)
{
    const int begin{ qFloor(hour) };
    // путь доступен, если он доступен в начале этого часа и доступен как минимум час
    const bool open{ trac.nearAvailable(begin, 1) == begin };

    if (open != en) {
        setTracEnabled(open);
        update();
    }
}

void TracGraphicsItem::setTracEnabled(bool enabled)
{
    en = enabled;
    auto p{ pen() };
    p.setColor(enabled ? QColor(0x03, 0xAC, 0x13) : QColor(0xff, 0x24, 0x00));
    setPen(p);
}
