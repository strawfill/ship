#ifndef PREPAREDDATA_H
#define PREPAREDDATA_H

#include <QMap>
#include <QVector>
#include <QPair>
#include <QPoint>
#include <QLine>
#include <QString>

#include "rawdata.h"

namespace prepared {

/*
  Icee поставим в Trac
  Mone раскидаем по Ship
  Ship разнесём на два класса
  Path особо не изменить

 */

enum ShipActivity
{
    sa_waiting = 0,
    sa_movement = 1,
    sa_layout = 2,
    sa_collection = 3,
    sa_shooting = 4,
};

constexpr auto formatTrac = raw::formatTrac;
constexpr auto formatShip = raw::formatShip;
constexpr auto formatMone = raw::formatMone;
constexpr auto formatIcee = raw::formatIcee;
constexpr auto formatPath = raw::formatPath;

// чтобы можно было в QMap запихнуть
struct Line : QLine
{
    Line() {}
    Line(const QPoint &pt1, const QPoint &pt2) : QLine(pt1, pt2) { }
    Line(int x1, int y1, int x2, int y2) : QLine(x1, y1, x2, y2) { }
    bool operator<(const Line &other) const;
};

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
    Trac() : limits({}) {}
    Trac(const raw::Trac &trac, const QVector<raw::Icee> &icees);

    bool valid() const { return  validData; }

    QPoint p1() const { return ln.p1(); }
    QPoint p2() const { return ln.p2(); }
    Line line() const { return ln; }
    double dist() const { return distance; }
    int sensors() const { return sensorCount; }

    int nearAvailable(int from, int hours) const { return limits.nearAvailable(from, hours); }

private:
    Line ln;
    double distance;
    int sensorCount;
    IceeArray limits;
    bool validData{ false };
};

class Handler
{
public:
    Handler() {}
    Handler(const raw::Ship &ship, const raw::SensorMone &sensorMone, const raw::ShipMone &shipMone);

    bool valid() const { return  validData; }

    QString name() const { return nm; }
    int speed() const { return spd; }
    int sensors() const { return sensorCount; }
    qlonglong cost(int days) const { return days*dailyCost; }

    // сделаем сравнение только по реально имеющим смысл параметрам, имя нас не интересует
    bool operator==(const Handler &other) const
    { return spd == other.spd && sensorCount == other.sensorCount && dailyCost == other.dailyCost; }

    bool better(const Handler &other) const;


private:
    QString nm;
    int spd{}, sensorCount{};
    qlonglong dailyCost{};
    bool validData{ false };
};

class Shooter
{
public:
    Shooter() {}
    Shooter(const raw::Ship &ship, const raw::ShipMone &shipMone);

    bool valid() const { return  validData; }

    QString name() const { return nm; }
    int speed() const { return spd; }
    qlonglong cost(int days) const { return days*dailyCost; }

    // сделаем сравнение только по реально имеющим смысл параметрам, имя нас не интересует
    bool operator==(const Shooter &other) const
    { return spd == other.spd && dailyCost == other.dailyCost; }

    bool better(const Shooter &other) const
    { return !(spd < other.spd || dailyCost > other.dailyCost); }

private:
    QString nm;
    int spd{};
    qlonglong dailyCost{};
    bool validData{ false };
};

struct DataStatic
{
    QVector<Trac> tracs;
    QVector<Handler> handlers;
    QVector<Shooter> shooters;

    DataStatic() {  }
    DataStatic(const raw::Data &data);

    // некоторыми кораблями вообще ничего не сделать (если датчиков меньше, чем нужно на любую из трасс),
    // или они просто являются дублями - такое стоит убрать
    void removeDummyShips();
    void detectErrors();

    Handler handlerViaName(const QString &name) const { return name2handler.value(name); }
    Shooter shooterViaName(const QString &name) const { return name2shooter.value(name); }
    Trac tracViaLine(const Line &line) const;

private:
    static QVector<QVector<raw::Icee> > iceeToNumbers(const raw::Data &data);

    template<typename T>
    static QString fullShipName(const T &sourceObject)
    { return raw::Ship::typeToQChar(sourceObject.type) + sourceObject.name; }

    static QMap<QString, raw::ShipMone> fullnameToMone(const raw::Data &data);

private:
    // чтобы быстро искать данные
    QMap<QString, Handler> name2handler;
    QMap<QString, Shooter> name2shooter;
    QMap<Line, Trac> line2trac;
};

using PathDot = raw::PathDot;
using Path = QVector<PathDot>;

struct DataDynamic
{
    Path pathShooter;
    Path pathHandler;
    QString shooterName;
    QString handlerName;
    bool has{ false };

    DataDynamic() {  }
    DataDynamic(const raw::Data &data);
};

qlonglong totalCost(const DataStatic &ds, const DataDynamic &dd);

}

#endif // PREPAREDDATA_H
