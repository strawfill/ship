#include "algobruteforce.h"

#include <QtMath>
#include "movestopathconverter.h"
#include <QElapsedTimer>

AlgoBruteForce::AlgoBruteForce(const prepared::DataStatic ads)
    : ds(ads)
{
    ds.removeDummyShips();
}

prepared::DataDynamic AlgoBruteForce::find()
{
    {
        QElapsedTimer tm; tm.start();
        //test
        int size{ ds.tracs.size() };
        int size2{ 2*size };
        std::vector<int> hplaces;
        hplaces.resize(size2);
        for (int i = 0; i < size2; ++i)
            hplaces.at(i) = i;
        qlonglong test0{}, test1{}, test2{};
        do {
            ++test0;
            int to = 1 << size2;
            for (int p = 0; p < to; ++p) {
                std::vector<int> splaces;
                splaces.resize(size);

                for (int i = 0; i < size; ++i)
                    splaces.at(i) = i;

                do {
                    ++test1;
                    int to = 1 << size;
                    for (int p = 0; p < to; ++p) {
                        ++test2;
                    }
                } while(std::next_permutation(splaces.begin(), splaces.end()));
            }
        }
        while(std::next_permutation(hplaces.begin(), hplaces.end()));
        qDebug() << tm.elapsed() << "ms" << "perm" << test1 << "all" << test2;
        qDebug() << "now wait" << test2 / 1000. / 610 << "s";
        qDebug() << "lol. Only" << test0;
    }

    QElapsedTimer tm; tm.start();
    //QElapsedTimer t1, t2, t3, t4, t5; t1.start(); t2.start(); t3.start(); t4.start(); t5.start();
    qint64 d0{},d1{},d2{},d3{},d4{},d5{},dd1{},dd2{},dd3{},dd4{},dd5{};
    int varvara{0};
    int time = INT_MAX;
    prepared::DataDynamic result;

    int size{ ds.tracs.size() };
    int size2{ 2*size };
    std::vector<int> hplaces;
    hplaces.resize(size2);
    for (int i = 0; i < size2; ++i)
        hplaces.at(i) = i;

    MovesToPathConverter converter{ds};



    // выбор кораблей 1
    for (int hi = 0; hi < ds.handlers.size(); ++hi) {
        // выбор кораблей 2
        for (int si = 0; si < ds.shooters.size(); ++si) {
            converter.setShips(ds.handlers.at(hi), ds.shooters.at(si));
            do {
                // конкретная комбинация выбора путей для раскладчика
                std::vector<int> hplacesfixed;
                hplacesfixed.resize(size2);
                std::transform(hplaces.begin(), hplaces.end(), hplacesfixed.begin(), [&size](int v){ return v % size; });
                // конкретный выбор с какой из двух сторон путей стартовать
                int to = 1 << size2;
                for (int p = 0; p < to; ++p) {
                    // здесь уже все варианты раскладчиков учитаны, теперь нужно добавить варианты шутеров
                    ShipMovesVector hmoves;
                    hmoves.reserve(size2);
                    for (int i = 0; i < size2; ++i) {
                        hmoves.append(ShipMove{ds.tracs.at(hplacesfixed.at(i)).line(), bool(p & (1<<i))});
                    }
                    std::vector<int> splaces;
                    splaces.resize(size);

                    for (int i = 0; i < size; ++i)
                        splaces.at(i) = i;

                    do {
                        // задано всё, кроме выбора направления
                        int to = 1 << size;
                        for (int p = 0; p < to; ++p) {
                            d0 = tm.nsecsElapsed();
                            // здесь уже вообще всё учитано
                            // но вложенность 4 цикла...
                            ShipMovesVector smoves;
                            smoves.reserve(size);
                            for (int i = 0; i < size; ++i) {
                                smoves.append(ShipMove{ds.tracs.at(splaces.at(i)).line(), bool(p & (1<<i))});
                            }
                            d1 = tm.nsecsElapsed() - d0;
                            ++varvara;
                            auto temp{ converter.createPath(hmoves, smoves)};
                            d2 = tm.nsecsElapsed() - d1;
                            if (temp.isValid() && temp.time < time) {
                                result = converter.createDD(temp);
                                time = temp.time;
                            }
                            d3 = tm.nsecsElapsed() - d2;
                            dd1 += d1;
                            dd2 += d2;
                            dd3 += d3;

                        }

                    } while(std::next_permutation(splaces.begin(), splaces.end()));

                }

            } while(std::next_permutation(hplaces.begin(), hplaces.end()));
        }
    }
    auto elaps = tm.elapsed();
    qDebug() << "AlgoBruteForce n =" << size << "with" << elaps << "ms, count:" << varvara;
    qDebug() << "h:" << ds.handlers.size() << "s:" << ds.shooters.size();
    qDebug() << "select h:" << result.handlerName << "s:" << result.shooterName;
    qDebug() << "time" << time;
    qDebug() << "cost" << prepared::totalCost(ds, result);
    qDebug() << "speed:" << double(varvara) / elaps;
    qDebug() << "deltas" << dd1 / 1e6 / varvara << dd2 / 1e6 / varvara << dd3 / 1e6 / varvara
             << "other" << elaps - dd2 / 1e6 / varvara;
    return result;
}
