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
    int opttime{INT_MAX};
    int time{INT_MAX};

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
        unic(hmoves, smoves);
    }
    int calculateHoursWithoutChangeOrder()
    {
        int result{ converter.calculateHours(hmoves, smoves) };
        //if (result < 100) {
        //    qDebug().noquote() << "BAD" << result << ":\n" << converter.createDD(hmoves, smoves).toString();
        //}
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

    if (!data.converter.handlerCanPassIt(data.hmoves)) {
        // отменить действие
        std::swap(data.hmoves[a], data.hmoves[b]);
        return false;
    }

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

prepared::DataDynamic AlgoAnnealing::find(double &progress)
{
    QElapsedTimer tm; tm.start(); int s0{}, s1{}, s2{}, s3{};

    int varvara{0};
    prepared::DataDynamic result;

    int size{ ds.tracs.size() };
    int size2{ 2*size };

    const auto tracsCrowded = sortViaCrowding(ds.tracs);


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

    data.calculateHours();

    s0 = data.time;

    if (tracsCrowded.size() > 2) {
        // это значит, что трассы иначально были скучены и мы их перетасовали
        // попытаемся этим воспользоваться
        for (double temperature = 10; temperature > 0.005; temperature *= 0.8) {
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

            data.fromOpt();
        }
    }
    s1 = data.time;


#if 0
    for (int i = 0; i < 10; ++i) {
        doChangeInitialInOp(data);
        //qDebug() << "init change at" << i << "with time" << data.time;
    }
#endif

#if 1
#if 1
    //for (int temperature = 10; temperature; temperature--) {
    for (double temperature = 10; temperature > 0.05; temperature *= 0.8) {
        for (int i = 0; i < 10000; ++i) {
            ++varvara;
            doChangePlaceMulty(data, temperature);
        }
        // будем идти от оптимального на цикле
        data.fromOpt();
    }
    s2 = data.time;
#endif
    for (double temperature = 10; temperature > 0.05; temperature *= 0.8) {
        for (int i = 0; i < 10000; ++i) {
            ++varvara;
            doChangePlace(data, temperature);
            doChangeDirection(data, temperature);
        }
        data.fromOpt();
    }
    s3 = data.time;
#endif

    data.fromOpt();
    data.calculateHours();
    result = data.getDD();

    //Q_ASSERT(totalHours(result) == time);


    auto elaps = tm.elapsed();
    qDebug() << "AlgoAnnealing n =" << size << "with" << elaps << "ms, count:" << varvara;
    qDebug() << "h:" << ds.handlers.size() << "s:" << ds.shooters.size();
    qDebug() << "select h:" << result.handlerName << "s:" << result.shooterName;
    qDebug() << "time" << data.time;
    if (totalHours(result) != data.time) {
        qDebug() << "SSSSSSSSSSHHHHHHHHHHHHHHHHHHHHIIIIIIIIIIIIIIIIIIIITTTTTTTTTTTTTTT" << totalHours(result) << data.time;
    }
    qDebug() << "cost" << prepared::totalCost(ds, result);
    qDebug() << "speed:" << double(varvara) / elaps;
    qDebug() << "ops time in h:" << s0 << s1 << s2 << s3 << "h";

    return result;
}
