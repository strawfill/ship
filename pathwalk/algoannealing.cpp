#include "algoannealing.h"

#include <QtMath>
#include "movestopathconverter.h"
#include "sortviacrowding.h"
#include <QElapsedTimer>

#include <QtConcurrent>

AlgoAnnealing::AlgoAnnealing(const prepared::DataStatic ads)
    : ds(ads)
{
    ds.removeDummyShips();
}

prepared::DataDynamic AlgoAnnealing::find()
{
    double test;
    return find(test);

    // TO DO TODO
    QAtomicInt t;

    using AtimicDouble = std::atomic<double>;
    AtimicDouble d;


}

struct TimePath
{
    QString path;
    int time;
    bool eq;

    bool operator<(const TimePath &other) const
    { return this->time < other.time; }
};


static void unic(const ShipMovesVector &init, ShipMovesVector &target) {
    static QVector<char> ch;
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
    int time{};
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
    b = qBound(a+1, int(a+(b-a)*p+1), data.hmoves.size());

    if (a == b)
        ++b;

    //qDebug() << "data" << a << b << b-a;

    for (int i = a; i < b; ++i)
        data.hmoves[i].setStartPoint(!data.hmoves.at(i).isStartP1());
    std::reverse(data.hmoves.begin()+a, data.hmoves.begin()+b);
    // действие 1 конец

    if (!data.converter.handlerCanPassIt(data.hmoves)) {
        // отменить действие
        std::reverse(data.hmoves.begin()+a, data.hmoves.begin()+b);
        for (int i = a; i < b; ++i)
            data.hmoves[i].reverseStartPoint();
        return false;
    }

    unic(data.hmoves, data.smoves);
    int timeNow = data.converter.calculateHours(data.hmoves, data.smoves);
    if (timeNow <= 0) {
        // отменить действие
        std::reverse(data.hmoves.begin()+a, data.hmoves.begin()+b);
        for (int i = a; i < b; ++i)
            data.hmoves[i].reverseStartPoint();
        return false;
    }

    if (timeNow < data.time) {
        data.opthmoves = data.hmoves;
    }

    if (timeNow > data.time) {
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
    data.time = timeNow;
    return true;
}


bool doChangePlace(AnnealingData &data, double temperature, int a, int b)
{
    if (a == b)
        return false;
    // действие начало
    std::swap(data.hmoves[a], data.hmoves[b]);
    // действие 1 конец

    if (!data.converter.handlerCanPassIt(data.hmoves)) {
        // отменить действие
        std::swap(data.hmoves[a], data.hmoves[b]);
        return false;
    }

    unic(data.hmoves, data.smoves);
    int timeNow = data.converter.calculateHours(data.hmoves, data.smoves);
    if (timeNow <= 0) {
        // отменить действие
        std::swap(data.hmoves[a], data.hmoves[b]);
        return false;
    }

    if (timeNow < data.time) {
        data.opthmoves = data.hmoves;
    }

    if (timeNow > data.time) {
        // p - вероятность удачи
        double p = qExp(-(timeNow-data.time)/temperature);
        // удача не прошла
        if (qrand()%10000 > p*10000) {
            // отменить действие
            std::swap(data.hmoves[a], data.hmoves[b]);
            return false;
        }
    }
    data.time = timeNow;
    return true;
}

bool doChangePlace(AnnealingData &data, double temperature)
{
    int a = qrand() % data.hmoves.size();
    int b = qrand() % data.hmoves.size();
    return doChangePlace(data, temperature, a, b);
}

bool doChangeDirection(AnnealingData &data, double temperature, int changeIndex)
{
    // действие начало
    data.hmoves[changeIndex].reverseStartPoint();
    // действие конец

    unic(data.hmoves, data.smoves);
    int timeNow = data.converter.calculateHours(data.hmoves, data.smoves);

    if (timeNow < data.time) {
        data.opthmoves = data.hmoves;
    }

    if (timeNow > data.time) {
        // p - вероятность удачи
        double p = qExp(-(timeNow-data.time)/temperature);
        // удача не прошла
        if (qrand()%100 > p*100) {
            // отменить действие
            data.hmoves[changeIndex].reverseStartPoint();
            return false;
        }
    }
    data.time = timeNow;
    return true;
}

inline bool doChangeDirection(AnnealingData &data, double temperature)
{
    return doChangeDirection(data, temperature, qrand() % data.hmoves.size());
}

void doChangeInitialInOp(AnnealingData &data)
{
    const int size2{ data.hmoves.size() };
    const int size{ size2 / 2 };

    // перемешаем трассы, чтобы получить новое начальное размещение трасс
    for (short i = 0; i < size; ++i)
        data.smoves[i] = ShipMove{short(i), false};

    for (int i = 0; i < size; ++i)
        std::swap(data.smoves[i], data.smoves[qrand() % size]);

    // протестируем новое начало
    for (short i = 0; i < size2; ++i)
        data.hmoves[i] = ShipMove{data.smoves.at(i/2).trac(), bool(qrand()%2)};

#if 1
    for (int temp = 10; temp > 1; --temp) {
        for (int i = 0; i < size2; ++i) {
            doChangeDirection(data, temp / 2, i);
        }
    }
#endif

    int timeNow = data.converter.calculateHours(data.hmoves, data.smoves);

    if (timeNow < data.time) {
        data.opthmoves = data.hmoves;
    }

    data.time = timeNow;

}


} // end anonymous namespace

prepared::DataDynamic AlgoAnnealing::find(double &progress)
{
    QElapsedTimer tm; tm.start(); int s0{}, s1{}, s2{};
#if 0
    // значит трассы находятся относительно далеко друг от друга
    qDebug() << "tracCrowding" << tracCrowding(ds.tracs);
    if (tracCrowding(ds.tracs) > 1.5) {
        auto sets{ findCompactSets(ds.tracs) };
        qDebug() << "d" << sets.size() <<  tm.elapsed() << "ms";
        //return {};

        QVector<prepared::Trac> tracs;
        tracs.reserve(ds.tracs.size());
        for (int i = 0; i < sets.size(); ++i) {
            for (int k = 0; k < sets.at(i).size(); ++k)
                tracs.append(ds.tracs.at(sets.at(i).at(k)));
        }
        ds.tracs = tracs;
    }
#endif

    int varvara{0};
    int time = INT_MAX;
    prepared::DataDynamic result;

    int size{ ds.tracs.size() };
    int size2{ 2*size };

    QElapsedTimer timer; timer.start();
    const auto tracsCrowded = sortViaCrowding(ds.tracs);
    qDebug() << "timer" << timer.nsecsElapsed() / 1e6 << "ms";


    MovesToPathConverter converter{ds};
    converter.setShips(ds.handlers.first(), ds.shooters.first());


    ShipMovesVector opthmoves;
    opthmoves.resize(size2);

    ShipMovesVector hmoves;
    hmoves.resize(size2);
    ShipMovesVector smoves;
    smoves.resize(size);

    AnnealingData data{ converter, opthmoves, hmoves, smoves };

    for (short i = 0; i < size2; ++i)
        hmoves[i] = ShipMove{short(i/2), bool(qrand()%2)};

    unic(hmoves, smoves);

    data.time = converter.calculateHours(data.hmoves, data.smoves);
    data.opthmoves = data.hmoves;
    s0 = data.time;

    if (tracsCrowded.size() > 100) {
        // это значит, что трассы иначально были скучены и мы их перетасовали
        // попытаемся этим воспользоваться
        for (double temperature = 5; temperature > 0.005; temperature *= 0.8) {
            for (int i = 0; i < 10000; ++i) {
                ++varvara;
                //doChangePlace(data, temperature);
                //doChangeDirection(data, temperature);
                const int set{ qrand() % tracsCrowded.size() };
                const auto tracs{ tracsCrowded.at(set) };
                const int a = qrand() % tracs.size();
                const int b = qrand() % tracs.size();
                const int d = qrand() % tracs.size();
                doChangePlace(data, temperature, a, b);
                doChangeDirection(data, temperature, d);
            }
        }
    }

#if 0
    for (int i = 0; i < 10; ++i) {
        doChangeInitialInOp(data);
        //qDebug() << "init change at" << i << "with time" << data.time;
    }
#endif

#if 0
#if 0
    //for (int temperature = 10; temperature; temperature--) {
    for (double temperature = 10; temperature > 0.05; temperature *= 0.8) {
        for (int i = 0; i < 10000; ++i) {
            ++varvara;
            doChangePlaceMulty(data, temperature);
        }
        // будем идти от оптимального на цикле
        data.hmoves = data.opthmoves;
    }
#endif
    for (double temperature = 10; temperature > 0.05; temperature *= 0.8) {
        for (int i = 0; i < 100000; ++i) {
            ++varvara;
            doChangePlace(data, temperature);
            doChangeDirection(data, temperature);
        }
    }
#endif

    unic(opthmoves, smoves);

    result = converter.createDD(opthmoves, smoves);
    time = converter.calculateHours(opthmoves, smoves);

    //Q_ASSERT(totalHours(result) == time);


    auto elaps = tm.elapsed();
    qDebug() << "AlgoAnnealing n =" << size << "with" << elaps << "ms, count:" << varvara;
    qDebug() << "h:" << ds.handlers.size() << "s:" << ds.shooters.size();
    qDebug() << "select h:" << result.handlerName << "s:" << result.shooterName;
    qDebug() << "time" << time;
    if (totalHours(result) != time) {
        qDebug() << "SSSSSSSSSSHHHHHHHHHHHHHHHHHHHHIIIIIIIIIIIIIIIIIIIITTTTTTTTTTTTTTT" << totalHours(result) << time;
    }
    qDebug() << "cost" << prepared::totalCost(ds, result);
    qDebug() << "speed:" << double(varvara) / elaps;
    qDebug() << "ops time in h:" << s0 << s1 << s2;

    return result;
}
