#include "algobruteforce.h"

#include <QtMath>
#include "movestopathconverter.h"
#include <QElapsedTimer>

AlgoBruteForce::AlgoBruteForce(const prepared::DataStatic ads)
    : ds(ads)
{
    ds.removeDummyShips();
}

static ShipMovesVector unic(const ShipMovesVector &init) {
    static QVector<char> ch;
    if (ch.size() != init.size()/2)
        ch.resize(init.size()/2);
    ShipMovesVector result;
    result.reserve(init.size()/2);
    for (const auto & el : init) {
        if (!ch.at(el.trac())) {
            result.append(el);
            ch[el.trac()] = 1;
        }
        else {
            ch[el.trac()] = 0;
        }
    }
    return result;
}


#define SET_PROGRESS(expr) \
    if (progress) \
        *progress = (expr)\


prepared::DataDynamic AlgoBruteForce::find(double *progress)
{
    QElapsedTimer tm; tm.start();

    int varvara{0};
    int time = INT_MAX;
    prepared::DataDynamic result;

    int size{ ds.tracs.size() };
    int size2{ 2*size };

    Q_ASSERT(size < 5);

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

    double progressAll = 1;
    qlonglong progressCur{};
    {
        progressAll *= ds.handlers.size();
        progressAll *= ds.shooters.size();

        qlonglong perm{1};
        int temp{ size2 };
        while (temp) {
            perm *= temp;
            --temp;
        }

        progressAll += perm / (1LL << size);
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
                SET_PROGRESS(++progressCur/progressAll);

                for (int i = 0; i < size2; ++i) {
                    hmoves[i] = ShipMove{short(hplaces.at(i)), false};
                }
                // конкретный выбор с какой из двух сторон путей стартовать
                int to = 1 << size2;
                for (int p = 0; p < to; ++p) {
                    // здесь уже все варианты раскладчиков учитаны, теперь нужно добавить варианты шутеров
                    for (int i = 0; i < size2; ++i) {
                        hmoves[i].setStartPoint(p & (1<<i));
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
                            // здесь уже вообще всё учитано
                            // но вложенность 4 цикла...
                            for (int i = 0; i < size; ++i) {
                                smoves[i].setStartPoint(p & (1<<i));
                            }
                            int hours{ converter.calculateHours(hmoves, smoves) };
                            ++varvara;


                            if (hours > 0 && hours < time) {
                                result = converter.createDD(hmoves, smoves);
                                time = hours;
                            }

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

    return result;
}
