#include "sortviacrowding.h"

#include "mtsfinder.h"

#include <QtMath>
#include <QDebug>

namespace {

// считает расстояние между двумя точками
double distance(const QPoint &p1, const QPoint &p2) {
    QPoint dist{ p1 - p2 };
    return qSqrt(qlonglong(dist.x())*dist.x() + qlonglong(dist.y())*dist.y());
};

// мера скученности трасс (насколько они близки друг к другу)
// отношение среднего расстояния между трассами к их средней длине
// считается для трасс [begin, end)
double tracCrowding(const QVector<prepared::Trac> &tracs, int begin=0, int end=-1)
{
    begin = qBound(0, begin, tracs.size()-1);
    end = end < 0 ? tracs.size() : qBound(0, end, tracs.size());
    const int size{ end-begin };

    if (size < 2)
        return 1.;

    double accumTracLen{ 0.L };
    for (int i = begin; i < end; ++i)
        accumTracLen += tracs.at(i).dist();

    double accumTracDist{ 0.L };
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

    return accumTracDist / accumTracLen;
}

// мера скученности трасс для нескольких наборов, возвращает среднюю величину скученности,
// домноженную на поправочный коэффициент
double tracCrowding(const Path &path, const QVector<prepared::Trac> &tracs)
{
    if (tracs.isEmpty())
        return 1.;

    double result{};

    for (const auto & subpath : qAsConst(path)) {
        QVector<prepared::Trac> tr;
        tr.reserve(subpath.size());
        for (auto index : qAsConst(subpath))
            tr.append(tracs.at(index));

        result += tracCrowding(tr);
    }
    result /= path.size();

    return result;
}



} // end anonymous namespace



Path sortViaCrowding(QVector<prepared::Trac> &tracs)
{
    if (tracCrowding(tracs) < 1.2)
        return {};

    MtsFinder finder(tracs);

    auto best = finder.createPath(1);
    double bestCrowding{ tracCrowding(best, tracs) };


    int to{ qMax(4, int(2 * qSqrt(tracs.size()))) };
    for (int i = 2; i < to; ++i) {
        auto temp = finder.createPath(i);
        // чтобы большой порядок разбиений не считался чем-то хорошим, введём поправочный коэффициент
        const int adjust{ 3 };
        const double adjust_res{ qSqrt(adjust*adjust + i-1) - adjust + 1 };
        double crowding{ tracCrowding(temp, tracs) * adjust_res };
        //qDebug() << "cr" << crowding << temp;
        // чем меньше скученность после разделения, тем лучше получаемый результат
        if (bestCrowding > crowding) {
            bestCrowding = crowding;
            best = temp;
        }
    }
    //qDebug() << "best" << bestCrowding << best;

    QVector<prepared::Trac> result;
    result.reserve(tracs.size());

    for (const auto & subpath : qAsConst(best)) {
        for (auto index : qAsConst(subpath)) {
            result.append(tracs.at(index));
        }
    }

    auto test = [&](){
        QSet<int> indexes;
        int r{};
        for (const auto & subpath : qAsConst(best)) {
            for (auto index : qAsConst(subpath)) {
                indexes.insert(index);
                ++r;
            }
        }
        if (indexes.size() != tracs.size()) {
            qDebug() << "!error 1" << Q_FUNC_INFO << indexes.size() << tracs.size() << indexes;
        }
        if (indexes.size() != r) {
            qDebug() << "!error 2" << Q_FUNC_INFO << indexes.size() << r << indexes << best;
        }

        return indexes.size() == tracs.size() && indexes.size() == r;
    };
    Q_ASSERT(test());


    tracs.swap(result);
    return best;
}
