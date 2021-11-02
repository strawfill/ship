#include "algobruteforce.h"

#include "movestopathconverter.h"
#include <QElapsedTimer>

AlgoBruteForce::AlgoBruteForce(const prepared::DataStatic ads)
    : ds(ads)
{
    ds.removeDummyShips();
}

prepared::DataDynamic AlgoBruteForce::find()
{
    QElapsedTimer tm; tm.start();
    qlonglong cost = LONG_LONG_MAX;
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
                ShipMovesVector hmoves;
                hmoves.reserve(size2);
                // конкретный выбор с какой из двух сторон путей стартовать
                int to = 1 << size2;
                for (int p = 0; p < to; ++p) {
                    // здесь уже все варианты раскладчиков учитаны, теперь нужно добавить варианты шутеров
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
                            // здесь уже вообще всё учитано
                            // но вложенность 4 цикла...
                            ShipMovesVector smoves;
                            smoves.reserve(size);
                            for (int i = 0; i < size; ++i) {
                                smoves.append(ShipMove{ds.tracs.at(splaces.at(i)).line(), bool(p & (1<<i))});
                            }

                            // а теперь можно и посчитать...
                            auto temp{ converter.createPath(hmoves, smoves)};
                            if (temp.isValid() && temp.cost < cost) {
                                result = converter.createDD(temp);
                                cost = temp.cost;
                            }

                        }

                    } while(std::next_permutation(splaces.begin(), splaces.end()));

                }

            } while(std::next_permutation(hplaces.begin(), hplaces.end()));
        }
    }
    qDebug() << "AlgoBruteForce n =" << size << "with" << tm.elapsed() << "ms";
    return result;
}
