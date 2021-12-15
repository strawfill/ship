#include "algoannealing.h"

#include <QtMath>
#include "movestopathconverter.h"
#include "sortviacrowding.h"
#include <QElapsedTimer>

AlgoAnnealing::AlgoAnnealing(const prepared::DataStatic ads)
    : ds(ads)
{
    ds.removeDummyShips();
}

static void unic(const ShipMovesVector &init, ShipMovesVector &target, QVector<char> &ch) {
    if (ch.size() != init.size()/2)
        ch.resize(init.size()/2);
    int cur{0};
    for (int i = 0; i < init.size(); ++i) {
        const auto & el{ init.at(i) };
        if (!ch.at(el.trac())) {
            target[cur++] = el;
            ch[el.trac()] = 1;
        }
        else {
            ch[el.trac()] = 0;
        }
    }
}

namespace {

struct AnnealingData
{
    MovesToPathConverter &converter;
    ShipMovesVector &opthmoves;
    ShipMovesVector &hmoves;
    ShipMovesVector &smoves;
    QVector<char> ch;
    int opttime{INT_MAX};
    int time{INT_MAX};

    AnnealingData(MovesToPathConverter &c, ShipMovesVector &opth, ShipMovesVector &h, ShipMovesVector &s)
        : converter(c), opthmoves(opth), hmoves(h), smoves(s) {}

    void initOpt()
    {
        opthmoves = hmoves;
        opttime = time;
    }
    void fromOpt()
    {
        hmoves = opthmoves;
        time = opttime;
    }
    void tryToOpt()
    {
        if (time < opttime) {
            initOpt();
        }
    }
    void updateSmoves()
    {
        unic(hmoves, smoves, ch);
    }
    int calculateHoursWithoutChangeOrder()
    {
        int result{ converter.calculateHours(hmoves, smoves) };
        if (result > 0) {
            time = result;
            tryToOpt();
        }
        return result;
    }
    int calculateHours()
    {
        updateSmoves();
        return calculateHoursWithoutChangeOrder();
    }
    prepared::DataDynamic getDD()
    {
        fromOpt();
        updateSmoves();
        return converter.createDD(hmoves, smoves);
    }
};


bool doChangePlaceMulty(AnnealingData &data, double temperature)
{
    // действие начало
    int a = qrand() % data.hmoves.size();
    int b = qrand() % data.hmoves.size();
    if (a > b)
        std::swap(a, b);

    double p = qExp(-(b-a)/temperature/100);
    // сузим интервал в зависимости от температуры

    // b может быть равен size, ведь он далее используется без включения
    b = qBound(a, int(a+(b-a)*p+1), data.hmoves.size());

    //qDebug() << "data" << a << b << b-a;

    for (int i = a; i < b; ++i)
        data.hmoves[i].setStartPoint(!data.hmoves.at(i).isStartP1());
    std::reverse(data.hmoves.begin()+a, data.hmoves.begin()+b);
    // действие 1 конец

    int timeBef = data.time;
    int timeNow = data.calculateHours();
    if (timeNow <= 0) {
        // отменить действие
        std::reverse(data.hmoves.begin()+a, data.hmoves.begin()+b);
        for (int i = a; i < b; ++i)
            data.hmoves[i].reverseStartPoint();
        return false;
    }

    if (timeNow > timeBef) {
        // p - вероятность удачи
        double p = qExp(-(timeNow-data.time)/temperature);
        // удача не прошла
        if (qrand()%10000 > p*10000) {
            // отменить действие
            std::reverse(data.hmoves.begin()+a, data.hmoves.begin()+b);
            for (int i = a; i < b; ++i)
                data.hmoves[i].reverseStartPoint();
            return false;
        }
    }
    data.tryToOpt();
    return true;
}


bool doChangePlace(AnnealingData &data, double temperature, int a, int b)
{
    if (a == b)
        return false;
    // действие начало
    std::swap(data.hmoves[a], data.hmoves[b]);
    // действие 1 конец

    int timeBef = data.time;
    int timeNow = data.calculateHours();
    if (timeNow <= 0) {
        // отменить действие
        std::swap(data.hmoves[a], data.hmoves[b]);
        return false;
    }
    if (timeNow > timeBef) {
        // p - вероятность удачи
        double p = qExp(-(timeNow-data.time)/temperature);
        // удача не прошла
        if (qrand()%10000 > p*10000) {
            // отменить действие
            std::swap(data.hmoves[a], data.hmoves[b]);
            return false;
        }
    }
    data.tryToOpt();
    return true;
}

bool doChangePlace(AnnealingData &data, double temperature)
{
    int a = qrand() % data.hmoves.size();
    int b = qrand() % data.hmoves.size();
    if (a > b)
        std::swap(a, b);

    double p = qExp(-(b-a)/temperature/100);
    // сузим интервал в зависимости от температуры

    if (qrand() % 2)
        a = qBound(0, int(b-(b-a)*p), b);
    else
        b = qBound(a, int(a+(b-a)*p), data.hmoves.size()-1);

    return doChangePlace(data, temperature, a, b);
}

bool doChangeDirection(AnnealingData &data, double temperature, int changeIndex)
{
    // действие начало
    data.hmoves[changeIndex].reverseStartPoint();
    // действие конец

    int timeBef = data.time;
    int timeNow = data.calculateHoursWithoutChangeOrder();

    if (timeNow > timeBef) {
        // p - вероятность удачи
        double p = qExp(-(timeNow-data.time)/temperature);
        // удача не прошла
        if (qrand()%100 > p*100) {
            // отменить действие
            data.hmoves[changeIndex].reverseStartPoint();
            return false;
        }
    }
    data.tryToOpt();
    return true;
}

inline bool doChangeDirection(AnnealingData &data, double temperature)
{
    return doChangeDirection(data, temperature, qrand() % data.hmoves.size());
}


} // end anonymous namespace



#define SET_PROGRESS(expr) \
    if (progress) \
        *progress = (expr)\


prepared::DataDynamic AlgoAnnealing::find(double *progress)
{
    QElapsedTimer tm; tm.start(); int s0{}, s1{}, s2{}, s3{}, s4{};

    int varvara{0};
    prepared::DataDynamic result;

    int size{ ds.tracs.size() };
    int size2{ 2*size };

    auto pathCrowded = pathViaCrowding(ds.tracs);


    MovesToPathConverter converter{ds};
    converter.setShips(ds.handlers.first(), ds.shooters.first());


    ShipMovesVector opthmoves;
    opthmoves.resize(size2);

    ShipMovesVector hmoves;
    hmoves.resize(size2);
    ShipMovesVector smoves;
    smoves.resize(size);

    AnnealingData data{ converter, opthmoves, hmoves, smoves };

    for (short i = 0; i < size; ++i) {
        bool direction{ bool(qrand()%2) };
        hmoves[2*i] = ShipMove{short(i), direction};
        hmoves[2*i+1] = ShipMove{short(i), !direction};
    }

    data.calculateHours();

    s0 = data.time;


    qlonglong progressCur{};
    double progressAll = 0;
    progressAll += calculations(100000, 10, 0.05, 0.8);
    progressAll += calculations(10000, 10, 0.05, 0.8);


    progressAll += ds.handlers.size() * ds.shooters.size();
    int result_handler_index{ 0 };
    int result_shooter_index{ 0 };
    qlonglong bestCost{ converter.calculateCost(hmoves, smoves) };
    // переберём все варианты кораблей
    for (int hi = 0; hi < ds.handlers.size(); ++hi) {
        for (int si = 0; si < ds.shooters.size(); ++si) {
            SET_PROGRESS(++progressCur/progressAll);
            converter.setShips(ds.handlers.at(hi), ds.shooters.at(si));
            auto currentCost{ converter.calculateCost(hmoves, smoves) };
            if (currentCost < bestCost) {
                bestCost = currentCost;
                result_handler_index = hi;
                result_shooter_index = si;
            }
        }
    }
    converter.setShips(ds.handlers.at(result_handler_index), ds.shooters.at(result_shooter_index));
    qDebug() << "result ships" << result_handler_index << result_shooter_index;


    if (pathCrowded.size() > 2) {
        // это значит, что трассы изначально были скучены и мы их перетасовали
        // попытаемся этим воспользоваться
        auto initTracOrder{ ds.tracs };
        auto bestPath{ pathCrowded };
        int bestTime{INT_MAX};
        int permutations{}; QElapsedTimer tt; tt.start();
        do {
            ++permutations;
            ds.tracs = applyPath(pathCrowded, initTracOrder);
            data.calculateHours();
            if (data.time < bestTime) {
                bestTime = data.time;
                bestPath = pathCrowded;
            }
        } while (permutations < 100000 && std::next_permutation(pathCrowded.begin(), pathCrowded.end()));
        qDebug() << "perms" << permutations << "with time" << tt.nsecsElapsed() / 1e6 << "ms"
                 << permutations * 1e6 / tt.nsecsElapsed() << "per ms" << "and all time" << tm.nsecsElapsed() / 1e6 << "ms";
        ds.tracs = applyPath(bestPath, initTracOrder);
        data.calculateHours();

        s1 = data.time;

        progressAll += calculations(30000, 10, 0.005, 0.8);

        for (double temperature = 10; temperature > 0.005; temperature *= 0.8) {
            for (int i = 0; i < 30000; ++i) {
                SET_PROGRESS(++progressCur/progressAll);
                ++varvara;
                //doChangePlace(data, temperature);
                //doChangeDirection(data, temperature);
                const int set{ qrand() % pathCrowded.size() };
                const auto tracs{ pathCrowded.at(set) };
                const int a = qrand() % tracs.size();
                const int b = qrand() % tracs.size();
                const int d = qrand() % tracs.size();
                doChangePlace(data, temperature, a, b);
                doChangeDirection(data, temperature, d);
            }

            data.fromOpt();
        }


        progressAll += ds.handlers.size() * ds.shooters.size();
        int result_handler_index{ 0 };
        int result_shooter_index{ 0 };
        qlonglong bestCost{ converter.calculateCost(hmoves, smoves) };
        // переберём все варианты кораблей ещё раз
        for (int hi = 0; hi < ds.handlers.size(); ++hi) {
            for (int si = 0; si < ds.shooters.size(); ++si) {
                SET_PROGRESS(++progressCur/progressAll);
                converter.setShips(ds.handlers.at(hi), ds.shooters.at(si));
                auto currentCost{ converter.calculateCost(hmoves, smoves) };
                if (currentCost < bestCost) {
                    bestCost = currentCost;
                    result_handler_index = hi;
                    result_shooter_index = si;
                }
            }
        }
        converter.setShips(ds.handlers.at(result_handler_index), ds.shooters.at(result_shooter_index));
        qDebug() << "result ships" << result_handler_index << result_shooter_index;

        s2 = data.time;
    }

#if 1
#if 1
    for (double temperature = 10; temperature > 0.05; temperature *= 0.8) {
        for (int i = 0; i < 100000; ++i) {
            SET_PROGRESS(++progressCur/progressAll);
            ++varvara;
            doChangePlaceMulty(data, temperature);
        }
        // будем идти от оптимального на цикле
        data.fromOpt();
    }
    s3 = data.time;
#endif
    for (double temperature = 10; temperature > 0.05; temperature *= 0.8) {
        for (int i = 0; i < 10000; ++i) {
            SET_PROGRESS(++progressCur/progressAll);
            ++varvara;
            doChangePlace(data, temperature);
            doChangeDirection(data, temperature);
        }
        data.fromOpt();
    }
    s4 = data.time;
#endif

    data.fromOpt();
    data.calculateHours();
    result = data.getDD();


    auto elaps = tm.elapsed();
    qDebug() << "AlgoAnnealing n =" << size << "with" << elaps << "ms, count:" << varvara << progressAll;
    qDebug() << "h:" << ds.handlers.size() << "s:" << ds.shooters.size();
    qDebug() << "select h:" << result.handlerName << "s:" << result.shooterName;
    qDebug() << "time" << data.time;
    qDebug() << "cost" << prepared::totalCost(ds, result);
    qDebug() << "speed:" << double(varvara) / elaps;
    qDebug() << "ops time in h:" << s0 << s1 << s2 << s3 << s4 << "h";

    return result;
}

int AlgoAnnealing::calculations(int itersForCurrentTemperature, double init, double stop, double mulstep)
{
    int result{};
    for (double temperature = init; temperature > stop; temperature *= mulstep)
        result += itersForCurrentTemperature;
    return result;
}
