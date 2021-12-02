#ifndef WORKER_H
#define WORKER_H

#include <QObject>

#include "prepareddata.h"

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr, double *progress=nullptr);

    void setData(const prepared::DataStatic &data) { ds = data; }
    prepared::DataDynamic getData() const { return dd; }
    prepared::DataStatic getInitData() const { return ds; }

    void setProgressWatcher(double *part) { progressParam = part; }


public slots:
    void start();
    void stop();


signals:
    void ended();

private:
    void progressUpdateTimeout();

private:
    prepared::DataDynamic dd;
    prepared::DataStatic ds;
    double *progressParam{ nullptr };
};

#endif // WORKER_H
