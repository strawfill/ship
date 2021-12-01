#include "algodummy.h"

#include <QtMath>
#include "movestopathconverter.h"
#include <QElapsedTimer>


AlgoDummy::AlgoDummy(const prepared::DataStatic ads)
    : ds(ads)
{
}

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

prepared::DataDynamic AlgoDummy::find()
{
    int time = INT_MAX;
    prepared::DataDynamic result;

    int size{ ds.tracs.size() };
    int size2{ 2*size };

    MovesToPathConverter converter{ds};

    std::vector<int> hplaces;
    hplaces.resize(unsigned(size2));

    std::vector<int> splaces;
    splaces.resize(unsigned(size));

    ShipMovesVector hmoves;
    hmoves.resize(size2);

    ShipMovesVector smoves;
    smoves.resize(size);

    converter.setShips(ds.handlers.constFirst(), ds.shooters.constFirst());

    for (int i = 0; i < size2; ++i)
        hplaces.at(unsigned(i)) = i/2;

    do {
        for (int i = 0; i < size2; ++i) {
            hmoves[i] = ShipMove{short(hplaces.at(unsigned(i))), false};
        }
        // конкретный выбор с какой из двух сторон путей стартовать
        int to = 1 << size2;
        for (int p = 0; p < to; ++p) {
            // здесь уже все варианты раскладчиков учитаны, теперь нужно добавить варианты шутеров
            for (int i = 0; i < size2; ++i) {
                hmoves[i].setStartPoint(p & (1<<i));
            }

            unic(hmoves, smoves);

            int hours{ converter.calculateHours(hmoves, smoves) };

            if (hours > 0 && hours < time) {
                result = converter.createDD(hmoves, smoves);
                time = hours;
            }
        }
    } while(std::next_permutation(hplaces.begin(), hplaces.end()));

    return result;
}
