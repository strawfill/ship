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


static ShipMovesVector unic(const ShipMovesVector &init) {
    static QVector<char> ch;
    if (ch.size() != init.size()/2)
        ch.resize(init.size()/2);
    ShipMovesVector result;
    result.reserve(init.size()/2);
    for (const auto & el : init) {
        if (!ch.at(el.tracNum)) {
            result.append(el);
            ch[el.tracNum] = 1;
        }
        else {
            ch[el.tracNum] = 0;
        }
    }
    return result;
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

    std::vector<int> hplaces;
    hplaces.resize(size2);

    std::vector<int> splaces;
    splaces.resize(size);


    ShipMovesVector hmoves;
    hmoves.resize(size2);

    ShipMovesVector smoves;
    smoves.resize(size);

    std::vector<std::vector<int> > splacesVariants;


    int permutations{1};
    int tempSize{size};
    while (tempSize) {
        permutations *= tempSize;
        --tempSize;
    }

    double progressStep;
    {
        // TO DO TODO
        // обнулим всё
        progress = 0.;

        qlonglong perm{1};
        int temp{ 2*size };
        while (temp) {
            perm *= temp;
            --temp;
        }

        progressStep = double(perm) / (1LL << size) / ds.handlers.size() / ds.shooters.size();
    }

    // выбор кораблей 1
    for (int hi = 0; hi < ds.handlers.size(); ++hi) {
        // выбор кораблей 2
        for (int si = 0; si < ds.shooters.size(); ++si) {
            converter.setShips(ds.handlers.at(hi), ds.shooters.at(si));
            for (int i = 0; i < size2; ++i)
                hplaces.at(i) = i/2;
            do {
                progress += progressStep;
                // проверим, что число сенсоров не будет отрицательным - нам такое не нужно
                if (!converter.handlerCanPassIt(hplaces)) {
                    continue;
                }

                for (int i = 0; i < size2; ++i) {
                    hmoves[i] = ShipMove{short(hplaces.at(i)), false};
                }
                // конкретный выбор с какой из двух сторон путей стартовать
                int to = 1 << size2;
                for (int p = 0; p < to; ++p) {

                    smoves = unic(hmoves);

                    int hours{ converter.calculateHours(hmoves, smoves) };
                    ++varvara;

                    if (hours > 0 && hours < time) {
                        result = converter.createDD(hmoves, smoves);
                        time = hours;
                    }
                }

            } while(std::next_permutation(hplaces.begin(), hplaces.end()));
        }
    }
    auto elaps = tm.elapsed();
    qDebug() << "AlgoAnnealing n =" << size << "with" << elaps << "ms, count:" << varvara;
    qDebug() << "h:" << ds.handlers.size() << "s:" << ds.shooters.size();
    qDebug() << "select h:" << result.handlerName << "s:" << result.shooterName;
    qDebug() << "time" << time;
    qDebug() << "cost" << prepared::totalCost(ds, result);
    qDebug() << "speed:" << double(varvara) / elaps;

    return result;
}
