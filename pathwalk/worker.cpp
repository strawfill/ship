#include "worker.h"

#include "algoannealing.h"
#include "algobruteforce.h"
#include "movestopathconverter.h"

#include <QtConcurrent>

Worker::Worker(QObject *parent, double *progress)
    : QObject(parent)
    , progressParam(progress)
{
}

namespace {

struct InputData
{
    prepared::DataStatic data;
    int seed;
    double *progress{ nullptr };
};

prepared::DataDynamic calculate(const InputData & source)
{
    qsrand(unsigned(source.seed));
#if 1
    return AlgoAnnealing(source.data).find(source.progress);
#else
    return AlgoBruteForce(source.data).find(source.progress);
#endif
}

} // anonymous namespace

void Worker::start()
{
    const int numThreads{ qMax(1, QThread::idealThreadCount()) };

    QVector<InputData> vector;
    vector.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        vector.append({ds, i});
    }

    vector.first().progress = progressParam;

    auto results = QtConcurrent::blockingMapped(vector, calculate);

    QVector<QPair<qlonglong, int> > costs;
    costs.reserve(results.size());
    for (const auto &dd : qAsConst(results)) {
        costs.append({prepared::totalCost(ds, dd), prepared::totalHours(dd)});
        qDebug() << "result ..." << costs.last() << "h" << prepared::totalHours(dd);
    }

    int index{ std::distance(costs.constBegin(), std::min_element(costs.constBegin(), costs.constEnd())) };

    dd = results.at(index);

    emit ended();
}

void Worker::stop()
{

}
