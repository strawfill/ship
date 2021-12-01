#include "worker.h"

#include "algoannealing.h"
#include "algobruteforce.h"
#include "movestopathconverter.h"

#include <QtConcurrent>

Worker::Worker(QObject *parent) : QObject(parent)
{
}

namespace {

struct InputData
{
    prepared::DataStatic data;
    int seed;
};

prepared::DataDynamic calculate(const InputData & source)
{
    qsrand(source.seed);
    return AlgoAnnealing(source.data).find();
}

} // anonymous namespace

void Worker::start()
{
    const int numThreads{ QThread::idealThreadCount() };
    qDebug() << "threads" << numThreads;

    QVector<InputData> vector;
    vector.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i) {
        vector.append({ds, i});
    }

    auto results = QtConcurrent::blockingMapped(vector, calculate);
    qDebug() << "we are champions";

    QVector<qlonglong> costs;
    costs.reserve(results.size());
    for (const auto &dd : qAsConst(results)) {
        costs.append(prepared::totalCost(ds, dd) + prepared::totalHours(dd));
        qDebug() << "result ..." << costs.last() << "h" << prepared::totalHours(dd);
    }

    int index{ std::distance(costs.constBegin(), std::min_element(costs.constBegin(), costs.constEnd())) };

    dd = results.at(index);

    emit ended();
}

void Worker::stop()
{

}
