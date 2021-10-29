#ifndef TRACGRAPHICSITEM_H
#define TRACGRAPHICSITEM_H

#include <QGraphicsLineItem>
#include "graphicsiteminterface.h"

#include "prepareddata.h"

class TracGraphicsItem : public QGraphicsLineItem, public GraphicsItemInterface
{
public:
    TracGraphicsItem(const prepared::Trac &atrac);

    void setHour(double hour) override;

private:
    // что путь в данный мемент не закрыт через Icee
    void setTracEnabled(bool enabled);

private:
    prepared::Trac trac;
    bool en{ false };
};

#endif // TRACGRAPHICSITEM_H
