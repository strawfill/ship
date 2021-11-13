#include "algoannealing.h"

#include <QtMath>
#include "movestopathconverter.h"
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
        if (!ch.at(el.tracNum)) {
            target[cur++] = el;
            ch[el.tracNum] = 1;
        }
        else {
            ch[el.tracNum] = 0;
        }
    }

}

prepared::DataDynamic AlgoAnnealing::find(double &progress)
{
    QElapsedTimer tm; tm.start();

    int varvara{0};
    int time = INT_MAX;
    prepared::DataDynamic result;

    int size{ ds.tracs.size() };
    int size2{ 2*size };



    MovesToPathConverter converter{ds};
    converter.setShips(ds.handlers.first(), ds.shooters.first());


    ShipMovesVector opthmoves;
    opthmoves.resize(size2);

    ShipMovesVector hmoves;
    hmoves.resize(size2);
    ShipMovesVector smoves;
    smoves.resize(size);

    for (short i = 0; i < size2; ++i)
        hmoves[i] = ShipMove{short(i/2), bool(qrand()%2)};

    unic(hmoves, smoves);

    int timeBefore = converter.calculateHours(hmoves, smoves);
    opthmoves = hmoves;

    for (int temp = 100; temp; --temp) {
        for (int i = 0; i < 1000; ++i) {
            ++varvara;
            {
                // действие 1 начало
                int a = qrand() % size2;
                int b = qrand() % size2;
                std::swap(hmoves[a], hmoves[b]);
                // действие 1 конец

                if (!converter.handlerCanPassIt(hmoves)) {
                    // отменить действие
                    std::swap(hmoves[a], hmoves[b]);
                    continue;
                }

                unic(hmoves, smoves);
                int timeNow = converter.calculateHours(hmoves, smoves);
                if (timeNow <= 0)
                    continue;

                if (timeNow < timeBefore) {
                    opthmoves = hmoves;
                }

                if (timeNow > timeBefore) {
                    // p - вероятность удачи
                    double p = exp(-double(timeNow-timeBefore)/temp);
                    // удача не прошла
                    if (qrand()%10000 > p*10000) {
                        // отменить действие
                        std::swap(hmoves[a], hmoves[b]);
                        continue;
                    }
                }
                timeBefore = timeNow;
            }
            {
                // действие 2 начало
                int a = qrand() % size2;
                hmoves[a].isP1Start = !hmoves[a].isP1Start;
                // действие 2 конец

                unic(hmoves, smoves);
                int timeNow = converter.calculateHours(hmoves, smoves);

                if (timeNow < timeBefore) {
                    opthmoves = hmoves;
                }

                if (timeNow > timeBefore) {
                    // p - вероятность удачи
                    double p = exp(-double(timeNow-timeBefore)/temp);
                    // удача не прошла
                    if (qrand()%100 > p*100) {
                        // отменить действие
                        hmoves[a].isP1Start = !hmoves[a].isP1Start;
                        continue;
                    }
                }
                timeBefore = timeNow;
            }
        }
    }

    unic(opthmoves, smoves);

    result = converter.createDD(opthmoves, smoves);
    time = converter.calculateHours(opthmoves, smoves);


    auto elaps = tm.elapsed();
    qDebug() << "AlgoAnnealing n =" << size << "with" << elaps << "ms, count:" << varvara;
    qDebug() << "h:" << ds.handlers.size() << "s:" << ds.shooters.size();
    qDebug() << "select h:" << result.handlerName << "s:" << result.shooterName;
    qDebug() << "time" << time;
    qDebug() << "cost" << prepared::totalCost(ds, result);
    qDebug() << "speed:" << double(varvara) / elaps;

    return result;
}
