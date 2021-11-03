#include "movestopathconverter.h"

#include <QtMath>

MovesToPathConverter::MovesToPathConverter(const prepared::DataStatic &ads)
    : ds(ads)
{
    const auto & trs{ ads.tracs };
    lineState.fill(0, trs.size());
    lineStateChanged.fill(0, trs.size());
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

void MovesToPathConverter::setShips(const prepared::Handler &ship1, const prepared::Shooter &ship2)
{
    handler = ship1;
    shooter = ship2;

    handlerInvSpeed = 1. / handler.speed();
    shooterInvSpeed = 1. / shooter.speed();
    handlerInvSpeed2 = handlerInvSpeed*handlerInvSpeed;
    shooterInvSpeed2 = shooterInvSpeed*shooterInvSpeed;
}

MovesToPathConverter::PathAndTime MovesToPathConverter::createPath(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
{
    clear();

    // позиции по последней записи в path
    QPoint hpos, spos;
    // часы по последней записи в path
    int hhour{0}, shour{0};
    // текущие индексы в векторе
    int hcur{0}, scur{0};
    // число записей в пути
    // сами записи
    pat.handlerPath.clear();
    pat.shooterPath.clear();
    pat.handlerPath.reserve(handlerVec.size() * 3);
    pat.shooterPath.reserve(handlerVec.size() * 3);
    // удобные функции
#if 0
#define addh(act) pat.handlerPath.append({hpos.x(), hpos.y(), hhour, act});
#define adds(act) pat.shooterPath.append({spos.x(), spos.y(), shour, act});
#else
    auto addh = [this, &hpos, &hhour](int act) {
        pat.handlerPath.append({hpos.x(), hpos.y(), hhour, act});
    };
    auto adds = [this, &spos, &shour](int act) {
        pat.shooterPath.append({spos.x(), spos.y(), shour, act});
    };
#endif
    // число оставшихся датчиков на укладчике
    int sensors{ handler.sensors() };

    while (true) {
        // детектор дедлока
        bool hasSomeActions{ false };
        // сначала пытаемся сделать всё, что может обработчик без помощи шутера, потом наоборот. Так и чередуем
        while (true) {
            if (hcur >= handlerVec.size())
                break;
            const auto input{ handlerVec.at(hcur) };
            char & calls = lineState[input.tracNum];
            if (calls != 0 && calls != 2) {
                // мы не можем обоработать эту трассу
                // отдаём работу шутеру, может он разблокирует её
                break;
            }
            const auto trac{ ds.tracs.at(input.tracNum) };
            // сначала нужно добраться
            if (hpos != input.first(ds.tracs)) {
                addh(prepared::sa_movement);
                QPoint delta{ hpos - input.first(ds.tracs) };
                hpos = input.first(ds.tracs);
                hhour += qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * handlerInvSpeed);
            }
            // теперь нужно подождать, когда отработает другое судно и/или откроется трасса
            int t1{ qMax(hhour, lineStateChanged.at(input.tracNum)) };
            int t2{ qMax(t1, trac.nearAvailable(t1, qCeil(trac.dist() * handlerInvSpeed))) };
            if (t2 > hhour) {
                addh(prepared::sa_waiting);
                hhour = t2;
            }
            if (calls == 0) {
                sensors -= trac.sensors();
                if (sensors < 0) {
                    // был задан некорректный путь
                    return {};
                }
                addh(prepared::sa_layout);
            }
            else {
                sensors += trac.sensors();
                addh(prepared::sa_collection);
            }
            hpos = input.second(ds.tracs);
            hhour += qCeil(trac.dist() * handlerInvSpeed);
            hasSomeActions = true;
            ++calls;
            ++hcur;
            lineStateChanged[input.tracNum] = hhour;
        }

        while (true) {
            if (scur >= shooterVec.size())
                break;
            const auto input{ shooterVec.at(scur) };
            char & calls = lineState[input.tracNum];
            if (calls != 1) {
                // мы не можем обоработать эту трассу
                // отдаём работу укладчику, может он разблокирует её
                break;
            }
            const auto trac{ ds.tracs.at(input.tracNum) };
            // сначала нужно добраться
            if (spos != input.first(ds.tracs)) {
                adds(prepared::sa_movement);
                QPoint delta{ spos - input.first(ds.tracs) };
                spos = input.first(ds.tracs);
                shour += qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * shooterInvSpeed);
            }
            // теперь нужно подождать, когда отработает другое судно и/или откроется трасса
            int t1{ qMax(shour, lineStateChanged.at(input.tracNum)) };
            int t2{ qMax(t1, trac.nearAvailable(t1, qCeil(trac.dist() * shooterInvSpeed))) };
            if (t2 > shour) {
                adds(prepared::sa_waiting);
                shour = t2;
            }
            adds(prepared::sa_shooting);
            spos = input.second(ds.tracs);
            shour += qCeil(trac.dist() * shooterInvSpeed);
            hasSomeActions = true;
            ++calls;
            ++scur;
            lineStateChanged[input.tracNum] = shour;
        }

        if (!hasSomeActions) {
            // проверим, почему мы ничего не делали
            if (scur >= shooterVec.size() && hcur >= handlerVec.size()) {
                // всё хорошо, можно выйти из цикла и продолжить обработку
                break;
            }

            // всё плохо
            return {};
        }
    }
    // но ещё нужно добавить записи возвращения домой
    if (!hpos.isNull()) {
        addh(prepared::sa_movement);
        hhour += qCeil(qSqrt(hpos.x()*hpos.x() + hpos.y()*hpos.y()) * handlerInvSpeed);
        hpos = QPoint{};
    }
    addh(prepared::sa_waiting);

    if (!spos.isNull()) {
        adds(prepared::sa_movement);
        shour += qCeil(qSqrt(spos.x()*spos.x() + spos.y()*spos.y()) * shooterInvSpeed);
        spos = QPoint{};
    }
    adds(prepared::sa_waiting);

    PathAndTime result;
    result.handlerPath = pat.handlerPath;
    result.shooterPath = pat.shooterPath;
    result.time = qMax(hhour, shour);
    return result;
}

#define YT 0


int MovesToPathConverter::calculateHours(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
{
    static std::vector<char> lineState;
    static std::vector<int> lineStateChanged;

    if (lineState.size() != ds.tracs.size()) {
        lineState.resize(ds.tracs.size(), 0);
        lineStateChanged.resize(ds.tracs.size(), 0);
    }

    // позиции по последней записи в path
    QPoint hpos, spos;
    // часы по последней записи в path
    int hhour{0}, shour{0};
    // текущие индексы в векторе
    int hcur{0}, scur{0};

    while (true) {
        // детектор дедлока
        bool hasSomeActions{ false };
        // сначала пытаемся сделать всё, что может обработчик без помощи шутера, потом наоборот. Так и чередуем
        while (true) {
            if (hcur >= handlerVec.size())
                break;
            const auto input{ handlerVec.at(hcur) };
            char & calls = lineState[input.tracNum];
            if (calls == 1) {
                // мы не можем обоработать эту трассу
                // отдаём работу шутеру, может он разблокирует её
                break;
            }
            const auto trac{ ds.tracs.at(input.tracNum) };
            const auto p1{ input.first(ds.tracs) };
            const auto p2{ input.second(ds.tracs) };
            // сначала нужно добраться
            if (hpos != p1) {
                QPoint delta{ hpos - p1 };
                hpos = p1;
#if YT
                hhour += isqrt((delta.x()*delta.x() + delta.y()*delta.y())*handlerInvSpeed2);
#else
                hhour += qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y())*handlerInvSpeed);
#endif
            }

            // теперь нужно подождать, когда отработает другое судно и/или откроется трасса
            int t1{ lineStateChanged.at(input.tracNum) };
            if (t1 > hhour)
                hhour = t1;
            int t2{ trac.nearAvailable(hhour, qCeil(trac.dist() * handlerInvSpeed)) };
            if (t2 > hhour)
                hhour = t2;

            hpos = p2;
            hhour += qCeil(trac.dist() * handlerInvSpeed);
            hasSomeActions = true;

            ++hcur;
            lineStateChanged[input.tracNum] = hhour;

            if (calls == 2) {
                calls = 0;
                lineStateChanged[input.tracNum] = 0;
            }
            else
                ++calls;
        }

        while (true) {
            if (scur >= shooterVec.size())
                break;
            const auto input{ shooterVec.at(scur) };
            char & calls = lineState[input.tracNum];
            if (calls != 1) {
                // мы не можем обоработать эту трассу
                // отдаём работу укладчику, может он разблокирует её
                break;
            }
            const auto trac{ ds.tracs.at(input.tracNum) };
            const auto p1{ input.first(ds.tracs) };
            const auto p2{ input.second(ds.tracs) };
            // сначала нужно добраться
            if (spos != p1) {
                QPoint delta{ spos - p1 };
                spos = p1;
#if YT
                shour += isqrt((delta.x()*delta.x() + delta.y()*delta.y())*shooterInvSpeed2);
#else
                shour += qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y())*shooterInvSpeed);
#endif
            }
            // теперь нужно подождать, когда отработает другое судно и/или откроется трасса
            int t1{ lineStateChanged.at(input.tracNum) };
            if (t1 > shour)
                shour = t1;
            int t2{ trac.nearAvailable(shour, qCeil(trac.dist() * shooterInvSpeed)) };
            if (t2 > shour)
                shour = t2;

            spos = p2;
            shour += qCeil(trac.dist() * shooterInvSpeed);
            hasSomeActions = true;
            ++calls;
            ++scur;
            lineStateChanged[input.tracNum] = shour;
        }

        if (!hasSomeActions) {
            // проверим, почему мы ничего не делали
            if (scur >= shooterVec.size() && hcur >= handlerVec.size()) {
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
    if (!hpos.isNull()) {
#if YT
        hhour += isqrt((hpos.x()*hpos.x() + hpos.y()*hpos.y())*handlerInvSpeed2);
#else
        hhour += qCeil(qSqrt(hpos.x()*hpos.x() + hpos.y()*hpos.y())*handlerInvSpeed);
#endif
    }

    if (!spos.isNull()) {
#if YT
        shour += isqrt((spos.x()*spos.x() + spos.y()*spos.y())*shooterInvSpeed2);
#else
        shour += qCeil(qSqrt(spos.x()*spos.x() + spos.y()*spos.y())*shooterInvSpeed);
#endif
    }

    return qMax(hhour, shour);
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

void MovesToPathConverter::clear()
{
    lineState.fill(0, lineState.size());
    lineStateChanged.fill(0, lineStateChanged.size());

}
