#include "movestopathconverter.h"

#include <QtMath>

MovesToPathConverter::MovesToPathConverter(const prepared::DataStatic &ads)
    : ds(ads)
{
    const auto & tracs{ ads.tracs };
    for (const auto & trac : tracs) {
        tracMap.insert(trac.line(), trac);
        lineStateMap.insert(trac.line(), 0);
        lineStateChangedMap.insert(trac.line(), 0);
    }
}

MovesToPathConverter::PathAndCost MovesToPathConverter::createPath(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
{
    clear();

    constexpr QPoint startPos{0,0};
    // позиции по последней записи в path
    QPoint hpos{startPos}, spos{startPos};
    // часы по последней записи в path
    int hhour{0}, shour{0};
    // умножение обычно происходит быстрее, чем деление, поэтому посчитаем обратную величину
    const double hspd = 1. / handler.speed();
    const double sspd = 1. / shooter.speed();
    // текущие индексы в векторе
    int hcur{0}, scur{0};
    // число записей в пути
    int hcnt{0}, scnt{0};
    // сами записи
    QString hpath, spath;
    // удобные функции
    auto addh = [&hpath, &hcnt, &hpos, &hhour](int act) {
        ++hcnt;
        hpath += QString{"%1 %2 %3 %4\n"}.arg(hpos.x()).arg(hpos.y()).arg(hhour).arg(act);
    };
    auto adds = [&spath, &scnt, &spos, &shour](int act) {
        ++scnt;
        spath += QString{"%1 %2 %3 %4\n"}.arg(spos.x()).arg(spos.y()).arg(shour).arg(act);
    };

    while (true) {
        // детектор дедлока
        bool hasSomeActions{ false };
        // сначала пытаемся сделать всё, что может обработчик без помощи шутера, потом наоборот. Так и чередуем
        while (true) {
            if (hcur >= handlerVec.size())
                break;
            const auto input{ handlerVec.at(hcur) };
            const auto trac{ tracMap.value(input.line) };
            // сначала нужно добраться
            if (hpos != input.first()) {
                addh(prepared::sa_movement);
                hpos = input.first();
                QPoint delta{ hpos - input.first() };
                int dt = qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * hspd);
                hhour = qMax(hhour, lineStateChangedMap.value(trac.line()));
                hhour += trac.nearAvailable(hhour, qCeil(trac.dist() * hspd)) + dt;
            }
            char & calls = lineStateMap[input.line];
            if (calls != 0 && calls != 2) {
                // мы не можем обоработать эту трассу
                // отдаём работу шутеру, может он разблокирует её
                break;
            }
            hasSomeActions = true;
            addh(calls == 0 ? prepared::sa_layout : prepared::sa_collection);
            hpos = input.second();
            hhour += qCeil(trac.dist() * hspd);
            ++calls;
            ++hcur;
            lineStateChangedMap[trac.line()] = hhour;
        }

        while (true) {
            if (scur >= shooterVec.size())
                break;
            const auto input{ shooterVec.at(scur) };
            const auto trac{ tracMap.value(input.line) };
            // сначала нужно добраться
            if (spos != input.first()) {
                adds(prepared::sa_movement);
                spos = input.first();
                QPoint delta{ spos - input.first() };
                int dt = qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * hspd);
                shour = qMax(shour, lineStateChangedMap.value(trac.line()));
                shour += trac.nearAvailable(shour, qCeil(trac.dist() * sspd)) + dt;
            }
            char & calls = lineStateMap[input.line];
            if (calls != 1) {
                // мы не можем обоработать эту трассу
                // отдаём работу укладчику, может он разблокирует её
                break;
            }
            hasSomeActions = true;
            adds(prepared::sa_shooting);
            spos = input.second();
            shour += qCeil(trac.dist() * sspd);
            ++calls;
            ++scur;
            lineStateChangedMap[trac.line()] = shour;
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
    if (hpos != startPos) {
        addh(prepared::sa_movement);
        hpos = startPos;
        QPoint delta{ hpos - startPos };
        hhour += qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * hspd);
    }
    addh(prepared::sa_waiting);
    if (spos != startPos) {
        adds(prepared::sa_movement);
        spos = startPos;
        QPoint delta{ spos - startPos };
        shour += qCeil(qSqrt(delta.x()*delta.x() + delta.y()*delta.y()) * sspd);
    }
    adds(prepared::sa_waiting);


    int maxH = qMax(hhour, shour);
    int days = qCeil(maxH / 24.);
    PathAndCost result;
    auto & s{ result.path };
    s += QString{"H %1 %2\n"}.arg(handler.name()).arg(hcnt);
    s += hpath;
    s += QString{"S %1 %2\n"}.arg(shooter.name()).arg(scnt);
    s += spath;
    result.cost = handler.cost(days) + shooter.cost(days);
    return result;
}

void MovesToPathConverter::clear()
{
    // не будем удалять, ведь процесс пересоздания словаря довольно медлителен
    // просто обнулим значения
    for (auto & key : lineStateMap)
        key = 0;
    for (auto & key : lineStateChangedMap)
        key = 0;

}
