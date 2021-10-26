#ifndef PREPAREDDATA_H
#define PREPAREDDATA_H

#include <QMap>
#include <QVector>
#include <QPair>
#include <QPoint>
#include <QString>

#include "rawdata.h"

namespace prepared {

/*
  Icee поставим в Trac
  Mone раскидаем по Ship
  Ship разнесём на два класса
  Path особо не изменить

 */

class IceeArray
{
public:
    // на вход массив ограничений для конкретной трассы
    IceeArray(const QVector<raw::Icee> &source);

    // возвращает ближайшее время от from, когда трасса будет доступна hours часов
    int nearAvailable(int from, int hours) const;

private:
    int findOpenIndexFrom(int hour) const;

private:
    using Data = QVector<QPair<int, int> >;
    Data closes;
};

class Trac
{
public:
    Trac(const raw::Trac &trac, const QVector<raw::Icee> &icees);

    QPoint p1() const { return point1; }
    QPoint p2() const { return point2; }
    double dist() const { return distance; }
    int sensors() const { return sensorCount; }

    int nearAvailable(int from, int hours) const { return limits.nearAvailable(from, hours); }

private:
    QPoint point1, point2;
    double distance;
    int sensorCount;
    IceeArray limits;
};

class Handler
{
public:
    Handler(const raw::Ship &ship, const raw::SensorMone &sensorMone, const raw::ShipMone &shipMone);

    QString name() const { return nm; }
    int speed() const { return spd; }
    int sensors() const { return sensorCount; }
    qlonglong cost(int days) const { return days*dailyCost; }

private:
    QString nm;
    int spd, sensorCount;
    qlonglong dailyCost;
};

class Shooter
{
public:
    Shooter(const raw::Ship &ship, const raw::ShipMone &shipMone);

    QString name() const { return nm; }
    int speed() const { return spd; }
    qlonglong cost(int days) const { return days*dailyCost; }

private:
    QString nm;
    int spd;
    qlonglong dailyCost;
};


}

#endif // PREPAREDDATA_H
