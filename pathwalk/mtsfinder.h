#ifndef MTSFINDER_H
#define MTSFINDER_H

#include "prepareddata.h"

#include <QPolygonF>
#include <QSet>
#include <QList>
#include <QMultiMap>
#include <QVector>
#include <QSet>
#include <utility>
#include <vector>

/**
 * @brief Класс поиска минимального остовного дерева и дальнейшего его разделения на подграфы
 * при исключении самых тяжёлых ребёр
 *
 * Использует алгоритм Крускала
 */
class MtsFinder
{
    using GraphEdge = std::pair<int, int>;
    using WeightedEdges = std::vector<std::pair<double, GraphEdge> >;

public:
    // находит минимальный по весу остов
    MtsFinder(const QVector<prepared::Trac> &tracs);
    void findMts(const QVector<prepared::Trac> &tracs);

    using SubPath = QVector<int>;
    using Path = QVector<SubPath>;

    // исключает вершины, чтобы на выходе получилось subPathCount подграфов
    Path createPath(int subPathCount) const;

private:
    void initCenterPoints(const QVector<prepared::Trac> &tracs);
    double weight(int index1, int index2) const;
    int vertices() const { return int(weightedEdges.size()) + 1; }

    QVector<QSet<int> > createPaths(const WeightedEdges &edges) const;

private:
    QPolygon centerPoints;
    WeightedEdges weightedEdges;
};


#endif // MTSFINDER_H
