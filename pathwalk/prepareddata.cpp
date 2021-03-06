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
    : ln(trac.x0, trac.y0, trac.x1, trac.y1)
    , limits(icees)
    , validData(true)
{
    distance = qSqrt(qlonglong(ln.dx())*ln.dx() + qlonglong(ln.dy())*ln.dy());
    sensorCount = qIsNull(distance) ? 1 : qCeil(distance / trac.layoutStep);
}

Handler::Handler(const QString &name, int speedKmH, int sensors, int sensorMoney, int shipMoney)
    : nm(name)
    , spd(speedKmH*1000)
    , sensorCount(sensors)
    , dailyCost(qlonglong(sensors) * sensorMoney + shipMoney)
    , validData(true)
{
}

Handler::Handler(const raw::Ship &ship, const raw::SensorMone &sensorMone, const raw::ShipMone &shipMone)
    : nm(ship.name)
    , spd(ship.speed*1000)
    , sensorCount(ship.maxSensorCount)
    , dailyCost(qlonglong(ship.maxSensorCount) * sensorMone.money() + shipMone.money)
    , validData(true)
{
    Q_ASSERT(ship.type == raw::Ship::Type::handler);
    Q_ASSERT(ship.name == shipMone.name);
    Q_ASSERT(sensorMone.valid());
}

bool Handler::better(const Handler &other) const
{
    // если количество дачиков не совпадает, то напрямую судить о том, лучше судно или нет - нельзя
    if (sensorCount != other.sensorCount)
        return false;

    bool worse{ spd < other.spd || dailyCost > other.dailyCost };
    return !worse;
}

Shooter::Shooter(const QString &name, int speedKmH, int shipMoney)
    : nm(name)
    , spd(speedKmH*1000)
    , dailyCost(shipMoney)
    , validData(true)
{
}

Shooter::Shooter(const raw::Ship &ship, const raw::ShipMone &shipMone)
    : nm(ship.name)
    , spd(ship.speed*1000)
    , dailyCost(shipMone.money)
    , validData(true)
{
    Q_ASSERT(ship.type == raw::Ship::Type::shooter);
    Q_ASSERT(ship.name == shipMone.name);
}

DataStatic::DataStatic(const raw::Data &data)
{
    tracs.reserve(data.trac.size());
    const auto & iceeInNums{ iceeToNumbers(data) };
    for (int i = 0; i < data.trac.size(); ++i) {
        const auto & obj{ Trac(data.trac.at(i), iceeInNums.at(i)) };
        tracs.append(obj);
        line2trac.insert(obj.line(), obj);
    }

    handlers.reserve(2 * data.ship.size() / 3);
    shooters.reserve(2 * data.ship.size() / 3);
    const auto & moneMap{ fullnameToMone(data) };
    for (int i = 0; i < data.ship.size(); ++i) {
        const auto & ship{ data.ship.at(i) };
        const auto & mone{ moneMap.value(fullShipName(ship)) };

        if (ship.type == raw::Ship::Type::handler) {
            const auto & obj{ Handler(ship, data.sensorMone, mone) };
            handlers.append(obj);
            name2handler.insert(obj.name(), obj);
        }
        else {
            const auto & obj{ Shooter(ship, mone) };
            shooters.append(obj);
            name2shooter.insert(obj.name(), obj);
        }
    }

    // не все ошибки было удобно детектировать при raw структурах, поэтому здесь есть ещё детектор...
    // (да, это костыль, такую детекцию нужно по-хорошему вынести в другое место)
    detectErrors();
}

void DataStatic::applyAddedTracs()
{
    line2trac.clear();
    for (const auto & trac : qAsConst(tracs))
        line2trac.insert(trac.line(), trac);
}

void DataStatic::applyAddedShips()
{
    name2handler.clear();
    name2shooter.clear();

    for (const auto & ship : qAsConst(handlers))
        name2handler.insert(ship.name(), ship);

    for (const auto & ship : qAsConst(shooters))
        name2shooter.insert(ship.name(), ship);
}

namespace {

template<typename T>
QVector<T> withoutCopies(const QVector<T> &source)
{
    QVector<T> result;
    result.reserve(source.size());
    // нет смысла оптимизировать, это короткий этап, работающий один раз
    for (int i = 0; i < source.size(); ++i) {
        bool has{ false };
        for (int k = 0; k < result.size(); ++k) {
            if (source.at(i) == result.at(k)) {
                has = true;
                break;
            }
        }
        if (!has)
            result.append(source.at(i));
    }
    return result;
}

template<typename T>
void removeWorse(QVector<T> &source)
{
    for (int i = source.size()-1; i >= 0; --i) {
        for (int k = source.size()-1; k >= 0; --k) {
            if (k == i)
                continue;

            if (source.at(k).better(source.at(i))) {
                source.remove(i);
                break;
            }
        }
    }
}

} // end anonymous namespace

void DataStatic::removeDummyShips()
{
    QVector<Handler> newh{ withoutCopies(handlers) };
    QVector<Shooter> news{ withoutCopies(shooters) };

    removeWorse(newh);
    removeWorse(news);

    int minSensors{ 0 };
    for (const auto & trac : qAsConst(tracs)) {
        if (minSensors < trac.sensors())
            minSensors = trac.sensors();
    }
    for (int i = handlers.size()-1; i >= 0; --i) {
        if (handlers.at(i).sensors() < minSensors)
            handlers.removeAt(i);
    }

    handlers = newh;
    shooters = news;

    // фактически можно сказать, что в этом месте объект теряет согласованность данных, ведь словари для быстрого
    // поиска кораблей остались прежними. Но меня всё устраивает
}

void DataStatic::detectErrors()
{
    // поиск ошибок вида:
    //  какая-то трасса настолько большая, что среди обработчиков нет такого, который смог бы обойти её
    int maxTrac{ 0 };
    for (const auto & trac : qAsConst(tracs)) {
        if (trac.sensors() > maxTrac)
            maxTrac = trac.sensors();
    }
    int maxHandler{ 0 };
    for (const auto & handler : qAsConst(handlers)) {
        if (handler.sensors() > maxHandler)
            maxHandler = handler.sensors();
    }

    if (maxTrac > maxHandler) {
        qWarning() << "В" << formatTrac << "присутствует трасса, для обработки которой нужно ("
                   << maxTrac << ") датчиков, но обработчик с максимальным количеством сенсоров имеет их лишь ("
                   << maxHandler << ")";
    }
}

Trac DataStatic::tracViaLine(const Line &line) const
{
    auto test{ line2trac.value(line) };
    if (test.valid())
        return test;
    // если мы в другом порядке задали две точки линии, то это всё та же линия
    return line2trac.value(Line{ line.p2(), line.p1() });
}

QVector<QVector<raw::Icee> > DataStatic::iceeToNumbers(const raw::Data &data)
{
    QVector<QVector<raw::Icee> > result;
    result.resize(data.trac.size());

    for (int i = 0; i < data.icee.size(); ++i) {
        const auto &ic{ data.icee.at(i) };
        const int index{ ic.trackNumber - 1 };
        Q_ASSERT(index >= 0 && index < data.trac.size());
        result[index].append(ic);
    }
    return result;
}

QMap<QString, raw::ShipMone> DataStatic::fullnameToMone(const raw::Data &data)
{
    QMap<QString, raw::ShipMone> result;
    for (int i = 0; i < data.shipMone.size(); ++i) {
        result.insert(fullShipName(data.shipMone.at(i)), data.shipMone.at(i));
    }
    return result;
}

DataDynamic::DataDynamic(const raw::Data &data)
{
    if (data.path.size() != 2)
        return;

    has = true;

    const auto &p{ data.path };

    const int h{ (p.at(0).type == raw::Ship::Type::handler ? 0 : 1) };
    const int s{ (p.at(0).type == raw::Ship::Type::shooter ? 0 : 1) };

    Q_ASSERT(p.at(h).type == raw::Ship::Type::handler);
    Q_ASSERT(p.at(s).type == raw::Ship::Type::shooter);

    pathHandler = p.at(h).path;
    handlerName = p.at(h).name;

    pathShooter = p.at(s).path;
    shooterName = p.at(s).name;
}

QString DataDynamic::toString() const
{
    QString result;
    result.reserve(100 + 15*(pathHandler.size()+pathShooter.size()));
    result += formatPath;
    result += "\n";
    result += QString{"H %1 %2\n"}.arg(handlerName).arg(pathHandler.size());
    for (const auto & p : qAsConst(pathHandler))
        result += QString{"%1 %2 %3 %4\n"}.arg(p.x).arg(p.y).arg(p.timeH).arg(p.activity);
    result += QString{"S %1 %2\n"}.arg(shooterName).arg(pathShooter.size());
    for (const auto & p : qAsConst(pathShooter))
        result += QString{"%1 %2 %3 %4\n"}.arg(p.x).arg(p.y).arg(p.timeH).arg(p.activity);
    result += "/\n";
    return result;
}

int totalHours(const DataDynamic &dd)
{
    int result{ 0 };

    auto from = [&result](const Path &p) {
        if (!p.isEmpty() && result < p.last().timeH)
            result = p.last().timeH;
    };

    from(dd.pathHandler);
    from(dd.pathShooter);

    return result;
}

int totalDays(const DataDynamic &dd)
{
    return qCeil(totalHours(dd) / 24.);
}

qlonglong totalCost(const DataStatic &ds, const DataDynamic &dd)
{
    if (!dd.has)
        return 0LL;

    int days{ totalDays(dd) };

    return ds.handlerViaName(dd.handlerName).cost(days) +
            ds.shooterViaName(dd.shooterName).cost(days);
}



} // end prepared namespace
