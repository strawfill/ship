#ifndef TRACGRAPHICSITEM_H
#define TRACGRAPHICSITEM_H


#include <QGraphicsLineItem>

#include "graphicsiteminterface.h"

#include "prepareddata.h"

#include <QPair>
#include <QVector>

using TracActions = QVector<QPair<int, int> >;

class TracGraphicsItem : public QGraphicsLineItem, public GraphicsItemInterface
{
public:
    TracGraphicsItem(const prepared::Trac &atrac, const TracActions &acts, double amodifier=1.);

    void setHour(double hour) override;

private:
    enum TracState {
        ts_after_start,
        ts_layout,
        ts_after_layout,
        ts_shooting,
        ts_after_shooting,
        ts_collection,
        ts_after_collection,
    };
    static QPen penForState(TracState state, bool enabled);
    TracState getState(double hour) const;

private:
    prepared::Trac trac;
    TracActions actions;
};

#endif // TRACGRAPHICSITEM_H
