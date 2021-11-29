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
    int time;
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


bool doChangePlace(AnnealingData &data, double temperature)
{
    // действие начало
    int a = qrand() % data.hmoves.size();
    int b = qrand() % data.hmoves.size();
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

// мера скученности трасс (насколько они близки друг к другу)
// отношение среднего расстояния между трассами к их средней длине
// считается для трасс [begin, end)
double tracCrowding(const QVector<prepared::Trac> &tracs, int begin=0, int end=-1)
{
    begin = qBound(0, begin, tracs.size()-1);
    end = end < 0 ? tracs.size() : qBound(0, end, tracs.size());
    const int size{ end-begin };

    QElapsedTimer tm; tm.start();
    if (size < 2)
        return 1.;

    // перемешаем точки, чтобы исключить фактор злоумышленного подбора плохих комбинаций


    double accumTracLen{ 0.L };
    for (int i = begin; i < end; ++i)
        accumTracLen += tracs.at(i).dist();

    auto distance = [](QPoint p1, QPoint p2) {
        auto dist{ p1 - p2 };
        //return double(qAbs(dist.x()) + qAbs(dist.y()));
        return qSqrt(dist.x()*dist.x() + dist.y()*dist.y());
    };
    double accumTracDist{ 0.L };
    //int lazyEnd{ qMax() };
    for (int i = begin; i < end; ++i) {
        for (int k = i+1; k < end; ++k) {
            accumTracDist += distance(tracs.at(i).p1(), tracs.at(k).p1());
            accumTracDist += distance(tracs.at(i).p1(), tracs.at(k).p2());
            accumTracDist += distance(tracs.at(i).p2(), tracs.at(k).p1());
            accumTracDist += distance(tracs.at(i).p2(), tracs.at(k).p2());
        }
    }

    accumTracLen /= size;
    accumTracDist /= size*(size-1)*2;

    double result = accumTracDist / accumTracLen;

    qDebug() << "tracCrowding" << result << "is" << tm.nsecsElapsed() / 1e6 << "ms ("
             << tracs.size() << "tracs )";
    return result;
}

double fastTracCrowding(const QVector<prepared::Trac> &tracs, int begin=0, int end=-1)
{
    begin = qBound(0, begin, tracs.size()-1);
    end = end < 0 ? tracs.size() : qBound(0, end, tracs.size());
    const int size{ end-begin };
    if (size < 10)
        return tracCrowding(tracs, begin, end);

    QElapsedTimer tm; tm.start();
    if (size < 2)
        return 1.;

    // перемешаем точки, чтобы исключить фактор злоумышленного подбора плохих комбинаций
    static QVector<prepared::Trac> tr;
    if (tr.size() != tracs.size())
        tr.resize(tracs.size());

    for (int i = 0; i < tracs.size(); ++i)
        std::swap(tr[i], tr[qrand() % tracs.size()]);


    double accumTracLen{ 0.L };
    for (int i = begin; i < end; ++i)
        accumTracLen += tracs.at(i).dist();

    auto distance = [](QPoint p1, QPoint p2) {
        auto dist{ p1 - p2 };
        //return double(qAbs(dist.x()) + qAbs(dist.y()));
        return qSqrt(dist.x()*dist.x() + dist.y()*dist.y());
    };
    // эта функция вызывается 1 раз и для 1К треков занимает не более
    // 5 ms времени на Ryzen 5600. Поэтому нет смысла что-то оптимизировать (убирать counter)
    double accumTracDist{ 0.L };
    qlonglong counts{};
    //int lazyEnd{ qMax() };
    for (int i = begin; i < qMin(begin+100, end); ++i) {
        for (int k = i+1; k < qMin(i+1+100, end); ++k) {
            ++counts;
            accumTracDist += distance(tracs.at(i).p1(), tracs.at(k).p2());
        }
    }

    accumTracLen /= size;
    accumTracDist /= counts;

    double result = accumTracDist / accumTracLen;

    qDebug() << "fastTracCrowding" << result << "is" << tm.nsecsElapsed() / 1e6 << "ms ("
             << tracs.size() << "tracs )";
    return result;
}

struct CompactSets
{
    using Centers = QVector<QPoint>;
    Centers tracCenters;
    using Data = QVector<QVector<int> >;
    Data indexes;
    int dist;
};

int computeDistance(CompactSets &s)
{
    static CompactSets::Centers centersOfSets;
    centersOfSets.clear();
    const int size{ s.indexes.size() };
    centersOfSets.resize(size);

    for (int i = 0; i < size; ++i) {
        QPoint p;
        for (int k = 0; k < s.indexes.at(i).size(); ++k) {
            p += s.tracCenters.at(s.indexes.at(i).at(k));
        }
        if (s.indexes.at(i).size())
            p /= s.indexes.at(i).size();
        else
            return 0;
        centersOfSets[i] = p;
    }

    auto distance = [](QPoint p1, QPoint p2) {
        auto dist{ p1 - p2 };
        return qSqrt(dist.x()*dist.x() + dist.y()*dist.y());
    };

    int counting{};
    double accumDistance{};
    for (int i = 0; i < size; ++i) {
        for (int k = i+1; k < size; ++k) {
            ++ counting;
            accumDistance += distance(centersOfSets.at(i), centersOfSets.at(k));
        }
    }

    //if (int(accumDistance / counting) > 80)
    //    qDebug() << "--" << int(accumDistance / counting) << centersOfSets;
    return int(accumDistance / counting);
}

int computeTracCrowding(CompactSets &s)
{
    int counting{};
    for (int i = 0; i < s.indexes.size(); ++i) {
        ++counting;

    }

    static CompactSets::Centers centersOfSets;
    centersOfSets.clear();
    const int size{ s.indexes.size() };
    centersOfSets.resize(size);

    for (int i = 0; i < size; ++i) {
        QPoint p;
        for (int k = 0; k < s.indexes.at(i).size(); ++k) {
            p += s.tracCenters.at(s.indexes.at(i).at(k));
        }
        if (s.indexes.at(i).size())
            p /= s.indexes.at(i).size();
        else
            return 0;
        centersOfSets[i] = p;
    }

    auto distance = [](QPoint p1, QPoint p2) {
        auto dist{ p1 - p2 };
        return qSqrt(dist.x()*dist.x() + dist.y()*dist.y());
    };

    //int counting{};
    double accumDistance{};
    for (int i = 0; i < size; ++i) {
        for (int k = i+1; k < size; ++k) {
            ++ counting;
            accumDistance += distance(centersOfSets.at(i), centersOfSets.at(k));
        }
    }

    return int(accumDistance / counting);
}

bool csDoReplaceSome(CompactSets &sets, double temperature)
{
    // действие начало
    int source = qrand() % sets.indexes.size();
    int target = qrand() % sets.indexes.size();
    if (source == target)
        return false;

    const auto & sourceIndexes{ sets.indexes.at(source) };
    const int sourceSize{ sourceIndexes.size() };

    if (!sourceSize)
        return false;

    int a = qrand() % sourceSize;
    int b = qrand() % sourceSize;

    if (a > b)
        std::swap(a, b);

    double p = qExp(-(target-source)/temperature/100);
    // сузим интервал в зависимости от температуры

    // b может быть равен size, ведь он далее используется без включения
    b = qBound(a+1, int(a+(b-a)*p+1), sourceSize);

    // нельзя полностью изымать данные
    if (sets.indexes.at(source).size() == b-a)
        return false;

    sets.indexes[target].append(sets.indexes.at(source).mid(a, b-a));
    sets.indexes[source].remove(a, b-a);

    int distNow = computeDistance(sets);
    if (distNow <= 0) {
        // отменить действие
        const int ta{ sets.indexes.at(target).size() - (b-a) };
        sets.indexes[source].append(sets.indexes.at(target).mid(ta, b-a));
        sets.indexes[target].remove(ta, b-a);
        return false;
    }

    // если это плохо
    if (distNow < sets.dist) {
        // p - вероятность удачи
        double p = qExp(-(distNow-sets.dist)/temperature);
        // удача не прошла
        if (qrand()%10000 > p*10000) {
            // отменить действие
            const int ta{ sets.indexes.at(target).size() - (b-a) };
            sets.indexes[source].append(sets.indexes.at(target).mid(ta, b-a));
            sets.indexes[target].remove(ta, b-a);
            return false;
        }
    }
    sets.dist = distNow;
    return true;
}

int findCompactSets(CompactSets &sets, int setCount)
{
    // зададим некое начальное положение
    sets.indexes.clear();
    sets.indexes.resize(setCount);
    for (int i = 0; i < setCount; ++i) {
        sets.indexes[i].reserve(sets.tracCenters.size() / setCount + 5);
    }
    for (int i = 0; i < sets.tracCenters.size(); ++i) {
        sets.indexes[i % setCount].append(i);
    }

    sets.dist = computeDistance(sets);

    //for (int temperature = 10; temperature; temperature--) {
    for (double temperature = 10; temperature > 0.05; temperature *= 0.8) {
        for (int i = 0; i < 20000; ++i) {
            csDoReplaceSome(sets, temperature);
        }
    }
    return computeDistance(sets);
}


CompactSets::Data findCompactSets(const QVector<prepared::Trac> &tracs)
{
    CompactSets result;
    CompactSets temp;
    // инициализируем центры
    temp.tracCenters.reserve(tracs.size());
    for (auto & trac : tracs) {
        temp.tracCenters.append( (trac.p1()+trac.p2()) / 2 );
    }

    int dist = findCompactSets(temp, 4);
    result = temp;
    return result.indexes;
    //qDebug() << "dcal" << dist << "sets:" << temp.indexes.size();
    const int maxSets{ qMax(4, tracs.size()/10) };
    for (int setCount = 3; setCount <= maxSets; ++setCount) {
        int distNow = findCompactSets(temp, setCount);
        //qDebug() << "dcal" << distNow <<  "sets:" << temp.indexes.size();
        if (distNow > dist) {
            dist = distNow;
            result = temp;
        }
    }

    return result.indexes;
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
#if 0
    for (int i = 0; i < 10; ++i) {
        doChangeInitialInOp(data);
        //qDebug() << "init change at" << i << "with time" << data.time;
    }
#endif

#if 1
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


    auto elaps = tm.elapsed();
    qDebug() << "AlgoAnnealing n =" << size << "with" << elaps << "ms, count:" << varvara;
    qDebug() << "h:" << ds.handlers.size() << "s:" << ds.shooters.size();
    qDebug() << "select h:" << result.handlerName << "s:" << result.shooterName;
    qDebug() << "time" << time;
    qDebug() << "cost" << prepared::totalCost(ds, result);
    qDebug() << "speed:" << double(varvara) / elaps;
    qDebug() << "ops time in h:" << s0 << s1 << s2;

    return result;
}
