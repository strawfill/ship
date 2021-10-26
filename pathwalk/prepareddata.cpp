#include "prepareddata.h"

#include <QtMath>

namespace prepared {


IceeArray::IceeArray(const QVector<raw::Icee> &source)
{
    Data temp;
    temp.reserve(source.size());
    for (const auto & icee : source)
        temp.append(qMakePair(icee.close, icee.open));
    std::sort(temp.begin(), temp.end());

    closes.reserve(source.size());

    if (!temp.isEmpty())
        closes.append(temp.first());
    for (int i = 1; i < temp.size(); ++i) {
        const auto & before{ closes.last() };
        const auto & current{ temp.at(i) };

        if (current.first <= before.second) {
            // уже ясно, что они пересекаются
            if (current.second <= before.second) {
                // следующий полностью поглощается текущим
                continue;
            }
            else {
                // следующий не полностью поглощается текущим, поэтому просто подправим дату
                closes.last().second = current.second;
            }
        }
        else {
            // не пересекается с предыдущим, просто добавим
            closes.append(current);
        }
    }
}

int IceeArray::nearAvailable(int from, int hours) const
{
    int openFromIndex{ findOpenIndexFrom(from) };
    // дальнейший алгоритм считает, что openFromIndex - индекс ограничения и его open находится слева от from и максимально близко к from

    int openHour{ from };

    while (true) {
        int nextIndex{ openFromIndex + 1 };

        // справа нет ограничений - смело берём текущее время
        if (nextIndex >= closes.size())
            return openHour;

        // получим время следующего ограничения и проверим, что мы успеваем
        int closeHour{ closes.at(nextIndex).first };
        if (closeHour - openHour >= hours)
            return openHour;

        openHour = closes.at(nextIndex).second;
        ++openFromIndex;
    }

}

// функция должна указывать на индекс последнего ограничения доступности, что его open слева и максимально близко к hour
// если же слева от hour нет ограничений, функция вернёт -1
int IceeArray::findOpenIndexFrom(int hour) const
{
    // проверка граничных случаев
    if (closes.isEmpty() || hour < closes.first().second)
        return -1;
    if (hour > closes.last().second)
        return closes.size()-1;

    // бинарный поиск
    int left{ 0 };
    int right{ closes.size()-1 };
    int current;
    while (true) {
        current = (right + left + 1) / 2;
        if (closes.at(current).second < hour)
            left = current;
        else
            right = current;

        if (right - left <= 1) {
            if (closes.at(right).second <= hour) // именно <=
                return right;
            else
                return left;
        }
    }
}

Trac::Trac(const raw::Trac &trac, const QVector<raw::Icee> &icees)
    : point1(trac.x0, trac.y0)
    , point2(trac.x1, trac.y1)
    , limits(icees)
{
    auto delta{ point2 - point1 };
    distance = qSqrt(delta.x()*delta.x() + delta.y()*delta.y());
    sensorCount = qIsNull(distance) ? 1 : qCeil(distance / trac.layoutStep);
}

Handler::Handler(const raw::Ship &ship, const raw::SensorMone &sensorMone, const raw::ShipMone &shipMone)
    : nm(ship.name)
    , spd(ship.speed)
    , sensorCount(ship.maxSensorCount)
    , dailyCost(qlonglong(ship.maxSensorCount) * sensorMone.money() + shipMone.money)
{
    Q_ASSERT(ship.type == raw::Ship::Type::handler);
    Q_ASSERT(ship.name == shipMone.name);
    Q_ASSERT(sensorMone.valid());
}

Shooter::Shooter(const raw::Ship &ship, const raw::ShipMone &shipMone)
    : nm(ship.name)
    , spd(ship.speed)
    , dailyCost(shipMone.money)
{
    Q_ASSERT(ship.type == raw::Ship::Type::handler);
    Q_ASSERT(ship.name == shipMone.name);
}


} // end prepared namespace
