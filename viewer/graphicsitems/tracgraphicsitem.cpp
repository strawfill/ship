#include "tracgraphicsitem.h"

#include <QPen>
#include <QtMath>


TracGraphicsItem::TracGraphicsItem(const prepared::Trac &atrac, const TracActions &acts)
    : QGraphicsLineItem(atrac.line().p1().x(), -atrac.line().p1().y(),
                        atrac.line().p2().x(), -atrac.line().p2().y())
    , trac(atrac)
    , actions(acts)
{
}


void TracGraphicsItem::setHour(double hour)
{
    auto state{ getState(hour) };
    auto p{ penForState(state) };

    if (pen() != p) {
        setPen(p);
        update();
    }
}

QPen TracGraphicsItem::penForState(TracState state)
{
    auto createpen = [](const QColor &c, Qt::PenStyle ps) {
        return QPen{ c, 3, ps, Qt::RoundCap };
    };

    switch (state) {
    case ts_disabled:   return createpen({0xff, 0x24, 0x00}, Qt::SolidLine);
    case ts_enabled:    return createpen({0x03, 0xAC, 0x13}, Qt::SolidLine);
    case ts_layout:     return createpen({0x00, 0x99, 0xCC}, Qt::DotLine);
    case ts_collection: return createpen({0x66, 0x00, 0x66}, Qt::DotLine);
    case ts_shooting:   return createpen({0xFF, 0x99, 0x33}, Qt::DotLine);
    case ts_after_disabled:   return createpen({0xff, 0x24, 0x00, 0x20}, Qt::SolidLine);
    case ts_after_enabled:    return createpen({0x03, 0xAC, 0x13, 0x20}, Qt::SolidLine);
    }
    return {};
}

TracGraphicsItem::TracState TracGraphicsItem::getState(double hour) const
{
    const int begin{ qFloor(hour) };
    // путь доступен, если он доступен в начале этого часа и доступен как минимум час
    const bool enabled{ trac.nearAvailable(begin, 1) == begin };

    if (actions.size() == 3) {

        if (hour > actions.at(2).second)
            return enabled ? ts_after_enabled : ts_after_disabled;

        if (hour >= actions.at(0).first && hour <= actions.at(0).second)
            return ts_layout;
        if (hour >= actions.at(1).first && hour <= actions.at(1).second)
            return ts_shooting;
        if (hour >= actions.at(2).first && hour <= actions.at(2).second)
            return ts_collection;

        return enabled ? ts_enabled : ts_disabled;

    }
    else {
        return enabled ? ts_enabled : ts_disabled;
    }
}
