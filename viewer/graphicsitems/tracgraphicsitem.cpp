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
    const int begin{ qFloor(hour) };
    // путь доступен, если он доступен в начале этого часа и доступен как минимум час
    const bool enabled{ trac.nearAvailable(begin, 1) == begin };
    auto p{ penForState(state, enabled) };

    if (pen() != p) {
        setPen(p);
        update();
    }
}

QPen TracGraphicsItem::penForState(TracState state, bool enabled)
{
    auto createpen = [](const QColor &c, Qt::PenStyle ps) {
        return QPen{ c, 3, ps, Qt::RoundCap };
    };

    switch (state) {
    case ts_after_start:
        return enabled ? createpen(QColor{0x03, 0xAC, 0x13}, Qt::SolidLine) :
                         createpen(QColor{0xff, 0x24, 0x00}, Qt::SolidLine);
    case ts_layout:
        return createpen(QColor{0x00, 0x99, 0xCC}, Qt::DotLine);
    case ts_after_layout:
        return createpen(QColor{0x00, 0x99, 0xCC, 0xA0}, Qt::DashLine);
    case ts_shooting:
        return createpen(QColor{0xFF, 0x99, 0x33}, Qt::DotLine);
    case ts_after_shooting:
        return createpen(QColor{0xFF, 0x99, 0x33, 0xA0}, Qt::DashLine);
    case ts_collection:
        return createpen(QColor{0x66, 0x00, 0x66}, Qt::DotLine);
    case ts_after_collection:
        return enabled ? createpen(QColor{0x03, 0xAC, 0x13, 0x20}, Qt::SolidLine) :
                         createpen(QColor{0xff, 0x24, 0x00, 0x20}, Qt::SolidLine);
    }
    return {};
}

TracGraphicsItem::TracState TracGraphicsItem::getState(double hour) const
{
    if (actions.size() != 3) {
        return ts_after_start;
    }

    if (hour < actions.at(0).first)
        return ts_after_start;
    if (hour < actions.at(0).second)
        return ts_layout;
    if (hour < actions.at(1).first)
        return ts_after_layout;
    if (hour < actions.at(1).second)
        return ts_shooting;
    if (hour < actions.at(2).first)
        return ts_after_shooting;
    if (hour < actions.at(2).second)
        return ts_collection;
    return ts_after_collection;

}
