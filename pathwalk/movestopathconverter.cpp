#include "movestopathconverter.h"

#include <QtMath>

MovesToPathConverter::MovesToPathConverter(const prepared::DataStatic &ads)
    : ds(ads)
{
    const auto & trs{ ads.tracs };
    lineState.fill(0, trs.size());
    lineStateChanged.fill(0, trs.size());
}

void MovesToPathConverter::setShips(const prepared::Handler &ship1, const prepared::Shooter &ship2)
{
    handler = ship1;
    shooter = ship2;

    handlerInvSpeed = 1. / handler.speed();
    shooterInvSpeed = 1. / shooter.speed();
}

MovesToPathConverter::PathAndTime MovesToPathConverter::createPath(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
{
    clear();

    constexpr QPoint startPos{0,0};
    // позиции по последней записи в path
    QPoint hpos{startPos}, spos{startPos};
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
    auto addh = [this, &hpos, &hhour](int act) {
        pat.handlerPath.append({hpos.x(), hpos.y(), hhour, act});
    };
    auto adds = [this, &spos, &shour](int act) {
        pat.shooterPath.append({spos.x(), spos.y(), shour, act});
    };
    // число оставшихся датчиков на укладчике
    int sensors{ handler.sensors() };

    while (true) {
        // детектор дедлока
        bool hasSomeActions{ false };
        // сначала пытаемся сделать всё, что может обработчик без помощи шутера, потом наоборот. Так и чередуем
        while (true) {
            if (Q_UNLIKELY(hcur >= handlerVec.size()))
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
            if (Q_LIKELY(hpos != input.first(ds.tracs))) {
                addh(prepared::sa_movement);
                QPoint delta{ hpos - input.first(ds.tracs) };
                int dt = qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * handlerInvSpeed);
                hpos = input.first(ds.tracs);
                hhour = trac.nearAvailable(hhour + dt, qCeil(trac.dist() * handlerInvSpeed));
            }
            int otherhour{ lineStateChanged.at(input.tracNum) };
            if (Q_UNLIKELY(otherhour > hhour)) {
                addh(prepared::sa_waiting);
                hhour = otherhour;
            }
            if (calls == 0) {
                sensors -= trac.sensors();
                if (Q_UNLIKELY(sensors < 0)) {
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
            if (Q_UNLIKELY(scur >= shooterVec.size()))
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
            if (Q_LIKELY(spos != input.first(ds.tracs))) {
                adds(prepared::sa_movement);
                QPoint delta{ spos - input.first(ds.tracs) };
                int dt = qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * shooterInvSpeed);
                spos = input.first(ds.tracs);
                shour = trac.nearAvailable(shour+dt, qCeil(trac.dist() * shooterInvSpeed));
            }
            int otherhour{ lineStateChanged.at(input.tracNum) };
            if (Q_UNLIKELY(otherhour > shour)) {
                adds(prepared::sa_waiting);
                shour = otherhour;
            }
            adds(prepared::sa_shooting);
            spos = input.second(ds.tracs);
            shour += qCeil(trac.dist() * shooterInvSpeed);
            hasSomeActions = true;
            ++calls;
            ++scur;
            lineStateChanged[input.tracNum] = shour;
        }

        if (Q_UNLIKELY(!hasSomeActions)) {
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
    if (hpos != startPos) {
        addh(prepared::sa_movement);
        QPoint delta{ hpos - startPos };
        hpos = startPos;
        hhour += qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * handlerInvSpeed);
    }
    addh(prepared::sa_waiting);

    if (spos != startPos) {
        adds(prepared::sa_movement);
        QPoint delta{ spos - startPos };
        spos = startPos;
        shour += qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * shooterInvSpeed);
    }
    adds(prepared::sa_waiting);

    PathAndTime result;
    result.handlerPath = pat.handlerPath;
    result.shooterPath = pat.shooterPath;
    result.time = qMax(hhour, shour);
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

void MovesToPathConverter::clear()
{
    lineState.fill(0, lineState.size());
    lineStateChanged.fill(0, lineStateChanged.size());

}
