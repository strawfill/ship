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
    TracGraphicsItem(const prepared::Trac &atrac, const TracActions &acts);

    void setHour(double hour) override;

private:
    enum TracState {
        ts_disabled,
        ts_enabled,
        ts_layout,
        ts_collection,
        ts_shooting,
        // когда все действия сделаны, аналоги первых двух
        ts_after_disabled,
        ts_after_enabled,
    };
    static QPen penForState(TracState state);
    TracState getState(double hour) const;

private:
    prepared::Trac trac;
    TracActions actions;
};

#endif // TRACGRAPHICSITEM_H
