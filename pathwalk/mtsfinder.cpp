#include "mtsfinder.h"

#include <QtMath>


#include <QDebug>
#include <QElapsedTimer>


namespace {
// далее идёт код алгоритма Крускала
// источник: https://www.geeksforgeeks.org/kruskals-minimum-spanning-tree-using-stl-in-c/
// были произведены небольшие правки алгорита из источника, чтобы убрать утечки памяти
// вроде как всё работает как нужно

using GraphEdge = std::pair<int, int>;
using WeightedEdges = std::vector<std::pair<double, GraphEdge> >;


class Graph
{
public:
    Graph(int V, int E) : V(V) { edges.reserve(unsigned(E)); }
    void addEdge(int u, int v, double w) { edges.push_back({w, {u, v}}); }

    WeightedEdges kruskalMST();

private:
    int V{};
    WeightedEdges edges;
};

struct DisjointSets
{
public:
    DisjointSets(int n)
    {
        ranks.resize(unsigned(n+1), 0);
        parents.resize(unsigned(n+1));
        for (unsigned i = 0; i <= unsigned(n); i++)
            parents[i] = i;
    }

    unsigned findParent(unsigned u)
    {
        // лень убирать рекурсию, и так всё быстро считается
        if (u != parents[u])
            parents[u] = findParent(parents[u]);
        return parents[u];
    }

    void merge(unsigned x, unsigned y)
    {
        x = findParent(x);
        y = findParent(y);

        if (ranks[x] > ranks[y])
            parents[y] = x;
        else
            parents[x] = y;

        if (ranks[x] == ranks[y])
            ++ranks[y];
    }

private:
    std::vector<unsigned> parents;
    std::vector<int> ranks;
};

WeightedEdges Graph::kruskalMST()
{
    WeightedEdges result;
    result.reserve(unsigned(V-1));

    sort(edges.begin(), edges.end());

    DisjointSets ds(V);

    for (auto it = edges.begin(); it != edges.end(); ++it)
    {
        unsigned u = unsigned(it->second.first);
        unsigned v = unsigned(it->second.second);

        unsigned set_u = ds.findParent(u);
        unsigned set_v = ds.findParent(v);

        if (set_u != set_v)
        {
            result.push_back(*it);

            if (result.size() == unsigned(V-1))
                break;

            ds.merge(set_u, set_v);
        }
    }

    return result;
}

} // end anonymous namespace





MtsFinder::MtsFinder(const QVector<prepared::Trac> &tracs)
{
    findMts(tracs);
}

void MtsFinder::findMts(const QVector<prepared::Trac> &tracs)
{

    initCenterPoints(tracs);

    const int v = tracs.size(); // число вершин
    const int e = v * (v-1) / 2; // число рёбер

    Graph graph(v, e);

    for (int i = 0; i < v; ++i) {
        for (int k = i+1; k < v; ++k) {
            graph.addEdge(i, k, weight(i, k));
        }
    }
    // далее центральные точки не нужны
    centerPoints.clear();
    centerPoints.squeeze();
    // получу результат
    weightedEdges = graph.kruskalMST();
}

void MtsFinder::initCenterPoints(const QVector<prepared::Trac> &tracs)
{
    centerPoints.resize(tracs.size());

    for (int i = 0; i < tracs.size(); ++i) {
        // вдруг их сумма переполнит int32, а точные координаты центров нам не важны
        centerPoints[i] = tracs.at(i).p1() / 2 + tracs.at(i).p2() / 2;
    }
}

double MtsFinder::weight(int index1, int index2) const
{
    Q_ASSERT(index1 >= 0 && index1 < centerPoints.size());
    Q_ASSERT(index2 >= 0 && index2 < centerPoints.size());

    const auto d{ centerPoints.at(index1) - centerPoints.at(index2) };

    return qSqrt(qlonglong(d.x())*d.x() + qlonglong(d.y())*d.y());
}

QVector<QSet<int> > MtsFinder::createPaths(const MtsFinder::WeightedEdges &edges) const
{
    if (edges.empty())
        return {};

    QVector<bool> matrix;
    const int size{ vertices() };
    matrix.resize(size*size);

    for (const auto & edge : qAsConst(edges)) {
        int row = edge.second.first;
        int column = edge.second.second;

        matrix[column*size + row] = true;
        matrix[row*size + column] = true;
    }

    QVector<int> toProcess;
    QVector<QSet<int> > result;

    QVector<int> usedColumns;
    usedColumns.resize(size);

    for (int i = 0; i < size; ++i) {
        if (!usedColumns.at(i)) {
            QSet<int> temp;
            toProcess.clear();
            toProcess.append(i);
            temp.insert(i);
            usedColumns[i] = true;

            while(!toProcess.isEmpty()) {
                int current = toProcess.takeFirst();
                for (int k = 0; k < size; ++k) {
                    if (!usedColumns.at(k) && matrix.at(k*size + current)) {
                        toProcess.append(k);
                        temp.insert(k);
                        usedColumns[k] = true;
                    }
                }
            }

            result.append(temp);
        }
    }

    return result;
}

MtsFinder::Path MtsFinder::createPath(int subPathCount) const
{
    // нужно разбить граф и получить точки
    auto copy{ weightedEdges };
    auto toRemove{ qMin(int(weightedEdges.size()), qMax(0, subPathCount-1)) };
    copy.erase(copy.end()-toRemove, copy.end());
    // после очищения мы могли в том числе потерять конкретные точки
    // если точка потеряна, значит она ни с кем не соединена и должна добавляться как одиночная

    auto temp{ createPaths(copy) };

    MtsFinder::Path result;

    for (const auto & set : qAsConst(temp)) {
        result.append(QVector<int>{set.constBegin(), set.constEnd()});
    }



    auto test = [&](){
        QSet<int> indexes;
        for (const auto & subpath : qAsConst(result)) {
            for (auto index : qAsConst(subpath)) {
                indexes.insert(index);
            }
        }
        if (indexes.size() != vertices()) {
            qDebug() << "!error 1" << Q_FUNC_INFO << indexes.size() << vertices() << indexes;
        }

        if (subPathCount != result.size()) {
            qDebug() << "!error 2" << Q_FUNC_INFO << subPathCount << result;

        }

        return indexes.size() == vertices() && subPathCount == result.size();
    };
    Q_ASSERT(test());

    return result;
}
