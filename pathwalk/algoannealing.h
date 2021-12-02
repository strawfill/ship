#ifndef ALGOANNEALING_H
#define ALGOANNEALING_H

#include "prepareddata.h"

class AlgoAnnealing
{
public:
    AlgoAnnealing(const prepared::DataStatic ads);

    prepared::DataDynamic find(double *progress=nullptr);

private:
    int calculations(int itersForCurrentTemperature, double init, double mulstep, double stop);

private:
    prepared::DataStatic ds;
};

#endif // ALGOANNEALING_H
