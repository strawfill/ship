#include "movestopathconverter.h"

#include <QtMath>

MovesToPathConverter::MovesToPathConverter(const prepared::DataStatic &ads)
    : ds(ads)
{
    lineState.resize(ads.tracs.size(), 0);
    lineStateChanged.resize(ads.tracs.size(), 0);
}

void MovesToPathConverter::setShips(const prepared::Handler &ship1, const prepared::Shooter &ship2)
{
    handler = ship1;
    shooter = ship2;
}

namespace {

struct ProcessTimeData
{
    const ShipMovesVector &moves;
    std::vector<char> &lineState;
    std::vector<int> &lineStateChanged;
    const prepared::DataStatic &ds;
    int speed;
    bool isHandler;
    QPoint pos;
    int hour{};
    int index{};
    int sensors{};

    enum : bool {
        handler = true,
        shooter = false,
    };

    void addGoHome()
    {
        if (!pos.isNull()) {
            hour += qCeil(qSqrt(qlonglong(pos.x())*pos.x() + qlonglong(pos.y())*pos.y()) / speed);
        }
    }

    bool atEnd() const { return index == moves.size(); }
};

bool processTime(ProcessTimeData &data)
{
    bool processed{ false };
    while (true) {
        if (data.index >= data.moves.size())
            break;
        const auto input{ data.moves.at(data.index) };
        char & calls = data.lineState[input.trac()];
        if (data.isHandler && calls == 1) {
            break;
        }
        if (!data.isHandler && calls != 1) {
            break;
        }
        processed = true;

        const auto trac{ data.ds.tracs.at(input.trac()) };
        const auto p1{ input.first(data.ds.tracs) };
        const auto p2{ input.second(data.ds.tracs) };
        // сначала нужно добраться
        if (data.pos != p1) {
            QPoint d{ data.pos - p1 };
            data.hour += qCeil(qSqrt(qlonglong(d.x())*d.x() + qlonglong(d.y())*d.y()) / data.speed);
            data.pos = p1;
        }

        // теперь нужно подождать, когда отработает другое судно и/или откроется трасса
        int &hourToOther = data.lineStateChanged.at(input.trac());
        if (hourToOther > data.hour)
            data.hour = hourToOther;
        int hourToOpen{ trac.nearAvailable(data.hour, qCeil(trac.dist() / data.speed)) };
        if (hourToOpen > data.hour)
            data.hour = hourToOpen;

        data.hour += qCeil(trac.dist() / data.speed);
        data.pos = p2;

        ++data.index;

        if (calls == 2) {
            calls = 0;
            hourToOther = 0;
            data.sensors += trac.sensors();
        }
        else {
            if (calls == 0) {
                data.sensors -= trac.sensors();
                if (data.sensors < 0) {
                    return false;
                }
            }

            ++data.lineState[input.trac()];
            hourToOther = data.hour;

        }
    }
    return processed;
}

}

int MovesToPathConverter::calculateHours(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
{
    if (lineState.size() != ds.tracs.size()) {
        lineState.resize(ds.tracs.size(), 0);
        lineStateChanged.resize(ds.tracs.size(), 0);
    }

    ProcessTimeData handler{ handlerVec, lineState, lineStateChanged, ds,
                this->handler.speed(), ProcessTimeData::handler };
    ProcessTimeData shooter{ shooterVec, lineState, lineStateChanged, ds,
                this->shooter.speed(), ProcessTimeData::shooter };
    handler.sensors = this->handler.sensors();

    while (true) {
        bool p1 = processTime(handler);
        bool p2 = processTime(shooter);

        if (handler.sensors < 0 || (!p1 && !p2)) {
            // проверим, почему мы ничего не делали
            if (handler.sensors >= 0 && shooter.atEnd() && handler.atEnd()) {
                // всё хорошо, можно выйти из цикла и продолжить обработку
                break;
            }
            // так как мы выходим, не до конца обработав входные данные, то обнулить их необходимо явно
            lineState.clear();
            lineStateChanged.clear();
            // всё плохо
            return -1;
        }
    }

    // но ещё нужно добавить записи возвращения домой
    handler.addGoHome();
    shooter.addGoHome();

    return qMax(shooter.hour, handler.hour);
}

qlonglong MovesToPathConverter::calculateCost(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
{
    int hours{ calculateHours(handlerVec, shooterVec) };
    if (hours <= -1)
        return -1;
    int days { qCeil(hours / 24.) };
    return handler.cost(days) + shooter.cost(days);
}

namespace {

struct ProcessPathData
{
    prepared::Path &path;
    const ShipMovesVector &moves;
    std::vector<char> &lineState;
    std::vector<int> &lineStateChanged;
    const prepared::DataStatic &ds;
    int speed;
    bool isHandler;
    QPoint pos;
    int hour{};
    int index{};

    enum : bool {
        handler = true,
        shooter = false,
    };

    void addAction(int act)
    {
        path.append({pos.x(), pos.y(), hour, act});
    }

    void addGoHome()
    {
        if (!pos.isNull()) {
            addAction(prepared::sa_movement);
            hour += qCeil(qSqrt(qlonglong(pos.x())*pos.x() + qlonglong(pos.y())*pos.y()) / speed);
            pos = QPoint{};
        }
        addAction(prepared::sa_waiting);
    }

    bool atEnd() const { return index == moves.size(); }
};

bool processPath(ProcessPathData &data)
{
    bool processed{ false };
    while (true) {
        if (data.index >= data.moves.size())
            break;
        const auto input{ data.moves.at(data.index) };
        char & calls = data.lineState[input.trac()];
        if (data.isHandler && calls == 1) {
            // если calls == 1, то эту трассу должен шутер обрабатывать
            break;
        }
        if (!data.isHandler && calls != 1) {
            // если calls != 1, то эту трассу должен не шутер обрабатывать
            break;
        }
        processed = true;

        const auto trac{ data.ds.tracs.at(input.trac()) };
        const auto p1{ input.first(data.ds.tracs) };
        const auto p2{ input.second(data.ds.tracs) };
        // сначала нужно добраться
        if (data.pos != p1) {
            data.addAction(prepared::sa_movement);
            QPoint d{ data.pos - p1 };
            data.hour += qCeil(qSqrt(qlonglong(d.x())*d.x() + qlonglong(d.y())*d.y()) / data.speed);
            data.pos = p1;
        }
        // теперь нужно подождать, когда отработает другое судно и/или откроется трасса
        int &hourToOther = data.lineStateChanged.at(input.trac());
        int t1{ qMax(data.hour, hourToOther) };
        int t2{ qMax(t1, trac.nearAvailable(t1, qCeil(trac.dist() / data.speed))) };
        if (t2 > data.hour) {
            data.addAction(prepared::sa_waiting);
            data.hour = t2;
        }
        // теперь и действия можно выполнить
        switch (calls) {
        case 0: data.addAction(prepared::sa_layout); break;
        case 1: data.addAction(prepared::sa_shooting); break;
        case 2: data.addAction(prepared::sa_collection); break;
        }

        data.hour += qCeil(trac.dist() / data.speed);
        data.pos = p2;

        ++data.index;

        if (calls == 2) {
            calls = 0;
            hourToOther = 0;
        }
        else {
            ++calls;
            hourToOther = data.hour;
        }
    }
    return processed;
}

}

MovesToPathConverter::PathAndTime
MovesToPathConverter::createPath(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
{
    lineState.clear();
    lineStateChanged.clear();
    lineState.resize(ds.tracs.size(), 0);
    lineStateChanged.resize(ds.tracs.size(), 0);

    pat.handlerPath.clear();
    pat.shooterPath.clear();

    ProcessPathData handler{ pat.handlerPath, handlerVec, lineState, lineStateChanged, ds,
                this->handler.speed(), ProcessPathData::handler };
    ProcessPathData shooter{ pat.shooterPath, shooterVec, lineState, lineStateChanged, ds,
                this->shooter.speed(), ProcessPathData::shooter };

    while (true) {
        bool p1 = processPath(handler);
        bool p2 = processPath(shooter);

        if (!p1 && !p2) {
            // проверим, почему мы ничего не делали
            if (shooter.atEnd() && handler.atEnd()) {
                // всё хорошо, можно выйти из цикла и продолжить обработку
                break;
            }
            // так как мы выходим, не до конца обработав входные данные, то обнулить их необходимо явно
            lineState.clear();
            lineStateChanged.clear();
            // всё плохо
            return {};
        }
    }

    // но ещё нужно добавить записи возвращения домой
    handler.addGoHome();
    shooter.addGoHome();

    PathAndTime result;
    result.handlerPath = pat.handlerPath;
    result.shooterPath = pat.shooterPath;
    result.time = qMax(handler.hour, shooter.hour);
    return result;
}

prepared::DataDynamic MovesToPathConverter::createDD(const PathAndTime &path)
{
    prepared::DataDynamic result;
    result.handlerName = handler.name();
    result.shooterName = shooter.name();
    result.pathHandler = path.handlerPath;
    result.pathShooter = path.shooterPath;
    result.has = true;
    return result;
}
