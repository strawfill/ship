#include "patherrordetector.h"

#include "prepareddata.h"

#include <QtMath>


using namespace prepared;

PathErrorDetector::PathErrorDetector(const prepared::DataStatic &staticData, const prepared::DataDynamic &dynamicData)
{
    setDataAndDetectErrors(staticData, dynamicData);
}

PathErrorDetector::~PathErrorDetector()
{
    clear();
}

void PathErrorDetector::setDataAndDetectErrors(const prepared::DataStatic &staticData, const prepared::DataDynamic &dynamicData)
{
    clear();
    sd = new DataStatic(staticData);
    dd = new DataDynamic(dynamicData);
    detectErrors();
}

void PathErrorDetector::clear()
{
    delete sd;
    sd = nullptr;

    delete dd;
    dd = nullptr;
}

void PathErrorDetector::detectErrors() const
{
    Q_ASSERT(dd && sd);

    if (!dd->has)
        return;

    detectStartEndErrors();
    detectSpeedAndTimeErrors();
    detectMismatchTracErrors();
    detectProcessingTracErrors();
    detectNoSensorsErrors();
}

void PathErrorDetector::detectStartEndErrors() const
{
    // поиск ошибок вида:
    //  корабли не начинают и/или не заканчивают маршрут в координатах (0, 0)
    //  действие корабля по завершению маршрута не простой

    auto detect = [](const QVector<PathDot> &pd) {
        if (!pd.isEmpty()) {
            if (pd.first().x != 0 || pd.first().y != 0) {
                qWarning() << "В" << formatPath << "судно должно стартовать из позиции (0 0), а не ("
                           << pd.first().x << pd.first().y << ")";
            }
            if (pd.last().x != 0 || pd.last().y != 0) {
                qWarning() << "В" << formatPath << "судно должно заканчивать свой маршрут в позиции (0 0), а не ("
                           << pd.last().x << pd.last().y << ")";
            }
            if (pd.last().activity != sa_waiting) {
                qWarning() << "В" << formatPath << "судно должно заканчивать свой маршрут с действием 0, а не ("
                           << pd.last().activity << ")";
            }
        }
    };

    detect(dd->pathHandler);
    detect(dd->pathShooter);
}

void PathErrorDetector::detectSpeedAndTimeErrors() const
{
    // поиск ошибок вида:
    //  время по записям в судне не возрастает
    //  судно изменило координаты после простоя
    //  судно двигалось быстрее реальной скорости
    detectSpeedAndTimeErrorsFor(dd->pathHandler, sd->handlerViaName(dd->handlerName).speed());
    detectSpeedAndTimeErrorsFor(dd->pathShooter, sd->shooterViaName(dd->shooterName).speed());
}

void PathErrorDetector::detectSpeedAndTimeErrorsFor(const QVector<PathDot> &pd, int speed) const
{
    for (int i = 1; i < pd.size(); ++i) {
        const auto & bef{ pd.at(i-1) };
        const auto & cur{ pd.at(i) };
        // проверим рост времени
        if (cur.timeH < bef.timeH) {
            qWarning() << "В" << formatPath << "ожидалось, что время не будет уменьшаться ("
                       << bef.x << bef.y << bef.timeH << bef.activity
                       << ") и (" << cur.x << cur.y << cur.timeH << cur.activity << ")";
        }
        // проверим на наличие движения (которого не должно быть) при простое
        if (bef.activity == sa_waiting) {
            if (bef.x != cur.x || bef.y != cur.y) {
                qWarning() << "В" << formatPath << "после записи о простое ("
                           << bef.x << bef.y << bef.timeH << bef.activity
                           << ") встречена запись с отличными от предыдущих координатами ("
                           << cur.x << cur.y << cur.timeH << cur.activity << ")";
            }
        }
        // проверим на превышение скорости
        const int dx{ cur.x - bef.x };
        const int dy{ cur.y - bef.y };
        const int deltaR2{ dx*dx + dy*dy };
        const int deltaH{ cur.timeH - bef.timeH };
        const int possibleR{ deltaH * speed };
        if (possibleR*possibleR < deltaR2) {
            qWarning() << "В" << formatPath << "судном превышена максимальная скорость между записями ("
                       << bef.x << bef.y << bef.timeH << bef.activity
                       << ") и (" << cur.x << cur.y << cur.timeH << cur.activity << ")"
                       << "допустимое расстояние (" << possibleR << ") между записями же ("
                       << qSqrt(deltaR2) << ")";
        }
    }
}

void PathErrorDetector::detectMismatchTracErrors() const
{
    // поиск ошибок вида:
    //   данный путь активного действия на самом деле не является существующей трассой
    detectMismatchTracErrorsFor(dd->pathHandler);
    detectMismatchTracErrorsFor(dd->pathShooter);
}

void PathErrorDetector::detectMismatchTracErrorsFor(const QVector<raw::PathDot> &pd) const
{
    for (int i = 1; i < pd.size(); ++i) {
        const auto & bef{ pd.at(i-1) };
        const auto & cur{ pd.at(i) };

        if (bef.activity == sa_layout || bef.activity == sa_collection || bef.activity == sa_shooting) {
            const Trac trac{ sd->tracViaLine(Line{ bef.x, bef.y, cur.x, cur.y }) };

            if (!trac.valid()) {
                qWarning() << "В" << formatPath << "активное действие (" <<  bef.activity
                           << ") происходит не по существующей трассе из" << formatTrac << "между записями ("
                           << bef.x << bef.y << bef.timeH << bef.activity
                           << ") и ("
                           << cur.x << cur.y << cur.timeH << cur.activity << ")";
            }
        }
    }
}

void PathErrorDetector::detectProcessingTracErrors() const
{
    // поиск ошибок вида:
    //   не все трассы пройдены в правильном порядке тремя действиями
    //   действия по обработке одной трассы пересекаются по времени

    // заполним для каждой трассы действия кораблей
    QMap<Line, QVector<QPair<PathDot, PathDot> > > map;

    for (const auto & trac : qAsConst(sd->tracs))
        map.insert(trac.line(), {});

    auto fillFrom = [&map, this](const QVector<PathDot> &pd) {
        for (int i = 1; i < pd.size(); ++i) {
            const auto & bef{ pd.at(i-1) };
            const auto & cur{ pd.at(i) };

            if (bef.activity == sa_layout || bef.activity == sa_collection || bef.activity == sa_shooting) {

                const Trac trac{ sd->tracViaLine(Line{ bef.x, bef.y, cur.x, cur.y }) };

                // это проверяется в другом месте, здесь же будет только мешать. Пропустим
                if (!trac.valid())
                    continue;

                // тут нет лишних действий, trac.line() не обязательно равен Line{ bef.x, bef.y, cur.x, cur.y }

                auto vector{ map.value(trac.line()) };
                vector.append({bef, cur});
                map.insert(trac.line(), vector);
            }
        }
    };

    fillFrom(dd->pathHandler);
    fillFrom(dd->pathShooter);


    for (const auto & trac : qAsConst(sd->tracs)) {
        const auto & line { trac.line() };
        const auto & v{ map.value(line) };

        if (v.size() != 3) {
            qWarning() << "В" << formatPath << "для трассы ("
                       << trac.p1().x() << trac.p1().y() << trac.p2().x() << trac.p2().y()
                       << ") ожидалось увидеть 3 взаимодействия с кораблями, но встречено (" << v.size() << ")";
            continue;
        }

        QSet<int> actions;
        for (int i = 0; i < v.size(); ++i) {
            actions << v.at(i).first.activity;
        }

        if (!actions.contains(sa_layout) || !actions.contains(sa_shooting) || !actions.contains(sa_collection)) {
            QStringList acts;
            for (const auto &a : qAsConst(actions))
                acts << QString::number(a);
            qWarning() << "В" << formatPath << "для трассы ("
                       << trac.p1().x() << trac.p1().y() << trac.p2().x() << trac.p2().y()
                       << ") ожидалось увидеть раскладку(2), прострел(4) и сбор(3), но встречены только дейстивия ("
                       << acts.join(" , ") << ")";
            continue;
        }

        // а теперь нужно проверить действия по времени
        using TimePair = QPair<int, int>;
        TimePair layout, shooting, collection;

        for (int i = 0; i < v.size(); ++i) {
            const auto pair{ v.at(i) };
            if (pair.first.activity == sa_layout)
                layout = { pair.first.timeH, pair.second.timeH };
            else if (pair.first.activity == sa_shooting)
                shooting = { pair.first.timeH, pair.second.timeH };
            else if (pair.first.activity == sa_collection)
                collection = { pair.first.timeH, pair.second.timeH };
        }

        if (shooting.first < layout.second) {
            qWarning() << "В" << formatPath << "для трассы ("
                       << trac.p1().x() << trac.p1().y() << trac.p2().x() << trac.p2().y()
                       << ") время начала прострела (" << shooting.first
                       << ") меньше времени завершения раскладки (" << layout.second << ")";
        }
        if (collection.first < shooting.second) {
            qWarning() << "В" << formatPath << "для трассы ("
                       << trac.p1().x() << trac.p1().y() << trac.p2().x() << trac.p2().y()
                       << ") время начала сбора датчиков (" << collection.first
                       << ") меньше времени завершения прострела (" << shooting.second << ")";
        }

        // и что в эти моменты трасса открыта
        auto testTracOpen = [&trac](const TimePair &pair, const QString &activityName) {
            if (trac.nearAvailable(pair.first, pair.second - pair.first) != pair.first) {
                qWarning() << "В" << formatPath << "трасса ("
                           << trac.p1().x() << trac.p1().y() << trac.p2().x() << trac.p2().y()
                           << ") при выполнении операции" << activityName
                           << "не всегда является доступной";
            }
        };

        testTracOpen(layout, "раскладка");
        testTracOpen(shooting, "прострел");
        testTracOpen(collection, "сбор");

    }


}

void PathErrorDetector::detectNoSensorsErrors() const
{
    // поиск ошибок вида:
    //   текущее количество датчиков уходит в минус

    int balance{ sd->handlerViaName(dd->handlerName).sensors() };
    int minBalance{ balance };

    for (int i = 1; i < dd->pathHandler.size(); ++i) {
        const auto & bef{ dd->pathHandler.at(i-1) };
        const auto & cur{ dd->pathHandler.at(i) };

        if (bef.activity == sa_layout) {
            const Trac trac{ sd->tracViaLine(Line{ bef.x, bef.y, cur.x, cur.y }) };
            balance -= trac.sensors();
            if (minBalance > balance)
                minBalance = balance;
        }
        else if (bef.activity == sa_collection) {
            const Trac trac{ sd->tracViaLine(Line{ bef.x, bef.y, cur.x, cur.y }) };
            balance += trac.sensors();
        }
    }

    if (minBalance < 0) {
        qWarning() << "В" << formatPath << "укладчику оказалось недостаточно датчиков для прохода по маршруту,"
                   << "максимально ему не хватало (" << -minBalance << ")";
    }
}
