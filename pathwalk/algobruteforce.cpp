#include "algobruteforce.h"

#include <QtMath>
#include "movestopathconverter.h"
#include <QElapsedTimer>

AlgoBruteForce::AlgoBruteForce(const prepared::DataStatic ads)
    : ds(ads)
{
    ds.removeDummyShips();
}

#define TTT 0

prepared::DataDynamic AlgoBruteForce::find()
{
    // std::next_permutation не требует много времени, нет смысла оптимизировать...
#if TTT
    {
        QElapsedTimer tm; tm.start();
        //test
        int size{ ds.tracs.size() };
        int size2{ 2*size };
        std::vector<int> hplaces;
        hplaces.resize(size2);
        for (int i = 0; i < size2; ++i)
            hplaces.at(i) = i/2;
        qlonglong test0{}, test01{0}, test1{}, test2{};
        do {
            ++test0;
            int to = 1 << size2;
            for (int p = 0; p < to; ++p) {
                ++test01;
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
        auto ws = test2 / 1000. / 2000;
        qDebug() << "now wait" << ws << "s" << "(" << ws / 60 << "m)";
        qDebug() << "lol. Only" << test0 << "but goto" << test01;
    }
#endif

    QElapsedTimer tm; tm.start();

#if TTT
    qint64 d0{},d1{},d2{},d3{},d4{},d5{},dd1{},dd2{},dd3{},dd4{},dd5{};
#endif
    int varvara{0}; int alexeevna{0}; int koroleva{0};
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

    splacesVariants.reserve(permutations);
    {
        std::vector<int> temp;
        temp.resize(size);
        int genTemp{-1};
        std::generate(temp.begin(), temp.end(), [&genTemp](){ return ++genTemp; } );
        do {
            splacesVariants.push_back(temp);
        } while (std::next_permutation(temp.begin(), temp.end()));
    }

    // выбор кораблей 1
    for (int hi = 0; hi < ds.handlers.size(); ++hi) {
        // выбор кораблей 2
        for (int si = 0; si < ds.shooters.size(); ++si) {
            converter.setShips(ds.handlers.at(hi), ds.shooters.at(si));
            for (int i = 0; i < size2; ++i)
                hplaces.at(i) = i/2;
            do {
                // проверим, что число сенсоров не будет отрицательным - нам такое не нужно
                if (!converter.handlerCanPassIt(hplaces)) {
                    continue;
                }
                ++koroleva;

                for (int i = 0; i < size2; ++i) {
                    hmoves[i] = ShipMove{short(hplaces.at(i)), false};
                }
                // конкретный выбор с какой из двух сторон путей стартовать
                int to = 1 << size2;
                for (int p = 0; p < to; ++p) {
                    // здесь уже все варианты раскладчиков учитаны, теперь нужно добавить варианты шутеров
                    for (int i = 0; i < size2; ++i) {
                        hmoves[i].isP1Start = p & (1<<i);
                    }

                    for (int i = 0; i < size; ++i)
                        splaces.at(i) = i;

                    for (int perm = 0; perm < permutations; ++perm) {
                        splaces = splacesVariants.at(perm);
                        // задано всё, кроме выбора направления
                        for (int i = 0; i < size; ++i) {
                            smoves[i] = ShipMove{short(splaces.at(i)), false};
                        }
                        int to = 1 << size;
                        for (int p = 0; p < to; ++p) {
#if TTT
                            d0 = tm.nsecsElapsed();
#endif
                            // здесь уже вообще всё учитано
                            // но вложенность 4 цикла...
                            for (int i = 0; i < size; ++i) {
                                smoves[i].isP1Start = p & (1<<i);
                            }
#if TTT
                            d1 = tm.nsecsElapsed() - d0;
#endif
                            ++varvara;
#if 0
                            int hours{ converter.calculateHours(hmoves, smoves) };
#else
                            auto temp{ converter.createPath(hmoves, smoves)};
                            int hours = temp.time;
#endif
#if TTT
                            d2 = tm.nsecsElapsed() - d1;
#endif
                            if (hours > 0 && hours < time) {
                                //qDebug() << "h" << hours << "bef" << time;
                                ++alexeevna;
                                result = converter.createDD(hmoves, smoves);
                                time = hours;
                            }
#if TTT
                            d3 = tm.nsecsElapsed() - d2;
                            dd1 += d1;
                            dd2 += d2;
                            dd3 += d3;
#endif

                        }

                    }

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
    qDebug() << "changes" << alexeevna;
    qDebug() << "real ops" << koroleva;
#if TTT
    qDebug() << "deltas" << dd1 / 1e6 / varvara << dd2 / 1e6 / varvara << dd3 / 1e6 / varvara
             << "other" << elaps - dd2 / 1e6 / varvara;
#endif
    return result;
}
