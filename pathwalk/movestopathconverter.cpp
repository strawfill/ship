﻿#include "movestopathconverter.h"

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

    handlerInvSpeed = 1. / handler.speed();
    shooterInvSpeed = 1. / shooter.speed();
}

bool MovesToPathConverter::handlerCanPassIt(const std::vector<int> handlerVec) const
{
    // не будем пересоздавать его, пусть всегда существует
    static std::vector<char> passItCheck;
    passItCheck.resize(handlerVec.size(), 0);
    int sensors{ handler.sensors() };

    for (unsigned i = 0; i < handlerVec.size(); ++i) {
        const auto trac{ ds.tracs.at(handlerVec.at(i)) };
        char & was = passItCheck.at(unsigned(handlerVec.at(i)));
        if (!was) {
            was = 1;
            sensors -= trac.sensors();
            if (sensors < 0) {
                passItCheck.clear();
                return false;
            }
        }
        else {
            was = 0;
            sensors += trac.sensors();
        }
    }
    return true;
}

namespace {

struct ProcessTimeData
{
    const ShipMovesVector &moves;
    std::vector<char> &lineState;
    std::vector<int> &lineStateChanged;
    const prepared::DataStatic &ds;
    double invSpeed;
    bool isHandler;
    QPoint pos;
    int hour{};
    int index{};

    enum : bool {
        handler = true,
        shooter = false,
    };

    void addGoHome()
    {
        if (!pos.isNull()) {
            hour += qCeil(qSqrt(pos.x()*pos.x() + pos.y()*pos.y())*invSpeed);
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
        char & calls = data.lineState[input.tracNum];
        if (data.isHandler && calls == 1) {
            break;
        }
        if (!data.isHandler && calls != 1) {
            break;
        }
        processed = true;

        const auto trac{ data.ds.tracs.at(input.tracNum) };
        const auto p1{ input.first(data.ds.tracs) };
        const auto p2{ input.second(data.ds.tracs) };
        // сначала нужно добраться
        if (data.pos != p1) {
            QPoint d{ data.pos - p1 };
            data.hour += qCeil(qSqrt(d.x()*d.x() + d.y()*d.y()) * data.invSpeed);
            data.pos = p1;
        }

        // теперь нужно подождать, когда отработает другое судно и/или откроется трасса
        int &hourToOther = data.lineStateChanged.at(input.tracNum);
        if (hourToOther > data.hour)
            data.hour = hourToOther;
        int hourToOpen{ trac.nearAvailable(data.hour, qCeil(trac.dist() * data.invSpeed)) };
        if (hourToOpen > data.hour)
            data.hour = hourToOpen;

        data.hour += qCeil(trac.dist() * data.invSpeed);
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

int MovesToPathConverter::calculateHours(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
{
    if (lineState.size() != ds.tracs.size()) {
        lineState.resize(ds.tracs.size(), 0);
        lineStateChanged.resize(ds.tracs.size(), 0);
    }

    ProcessTimeData handler{ handlerVec, lineState, lineStateChanged, ds,
                handlerInvSpeed, ProcessTimeData::handler };
    ProcessTimeData shooter{ shooterVec, lineState, lineStateChanged, ds,
                shooterInvSpeed, ProcessTimeData::shooter };

    while (true) {
        bool p1 = processTime(handler);
        bool p2 = processTime(shooter);

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
            return -1;
        }
    }

    // но ещё нужно добавить записи возвращения домой
    handler.addGoHome();
    shooter.addGoHome();

    return qMax(shooter.hour, handler.hour);
}

namespace {

struct ProcessPathData
{
    prepared::Path &path;
    const ShipMovesVector &moves;
    std::vector<char> &lineState;
    std::vector<int> &lineStateChanged;
    const prepared::DataStatic &ds;
    double invSpeed;
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
    };

    void addGoHome()
    {
        if (!pos.isNull()) {
            hour += qCeil(qSqrt(pos.x()*pos.x() + pos.y()*pos.y())*invSpeed);
        }

        if (!pos.isNull()) {
            addAction(prepared::sa_movement);
            hour += qCeil(qSqrt(pos.x()*pos.x() + pos.y()*pos.y()) * invSpeed);
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
        char & calls = data.lineState[input.tracNum];
        if (data.isHandler && calls == 1) {
            break;
        }
        if (!data.isHandler && calls != 1) {
            break;
        }
        processed = true;

        const auto trac{ data.ds.tracs.at(input.tracNum) };
        const auto p1{ input.first(data.ds.tracs) };
        const auto p2{ input.second(data.ds.tracs) };
        // сначала нужно добраться
        if (data.pos != p1) {
            data.addAction(prepared::sa_movement);
            QPoint d{ data.pos - p1 };
            data.hour += qCeil(qSqrt(d.x()*d.x() + d.y()*d.y()) * data.invSpeed);
            data.pos = p1;
        }
        // теперь нужно подождать, когда отработает другое судно и/или откроется трасса
        int &hourToOther = data.lineStateChanged.at(input.tracNum);
        int t1{ qMax(data.hour, hourToOther) };
        int t2{ qMax(t1, trac.nearAvailable(t1, qCeil(trac.dist() * data.invSpeed))) };
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

        data.hour += qCeil(trac.dist() * data.invSpeed);
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
    if (lineState.size() != ds.tracs.size()) {
        lineState.resize(ds.tracs.size(), 0);
        lineStateChanged.resize(ds.tracs.size(), 0);
    }

    pat.handlerPath.clear();
    pat.shooterPath.clear();

    ProcessPathData handler{ pat.handlerPath, handlerVec, lineState, lineStateChanged, ds,
                handlerInvSpeed, ProcessPathData::handler };
    ProcessPathData shooter{ pat.shooterPath, shooterVec, lineState, lineStateChanged, ds,
                shooterInvSpeed, ProcessPathData::shooter };

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

MovesToPathConverter::StringPathAndCost MovesToPathConverter::createQStringPath(const PathAndTime &path)
{
    StringPathAndCost result;
    auto & s{ result.path };
    s.reserve(15*(path.handlerPath.size()+path.shooterPath.size()));
    s += QString{"H %1 %2\n"}.arg(handler.name()).arg(path.handlerPath.size());
    for (const auto & p : qAsConst(path.handlerPath))
        s += QString{"%1 %2 %3 %4\n"}.arg(p.x).arg(p.y).arg(p.timeH).arg(p.activity);
    s += QString{"S %1 %2\n"}.arg(shooter.name()).arg(path.shooterPath.size());
    for (const auto & p : qAsConst(path.shooterPath))
        s += QString{"%1 %2 %3 %4\n"}.arg(p.x).arg(p.y).arg(p.timeH).arg(p.activity);
    result.cost = path.time;
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
