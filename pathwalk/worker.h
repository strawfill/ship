#ifndef WORKER_H
#define WORKER_H

#include <QObject>

#include "prepareddata.h"

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = nullptr);

    void setData(const prepared::DataStatic &data) { ds = data; }
    prepared::DataDynamic getData() const { return dd; }
    prepared::DataStatic getInitData() const { return ds; }


public slots:
    void start();
    void stop();


signals:
    void progressChanged(int percents);
    void ended();

private:
    prepared::DataDynamic dd;
    prepared::DataStatic ds;
};

#endif // WORKER_H
