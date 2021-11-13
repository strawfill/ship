#ifndef ALGOANNEALING_H
#define ALGOANNEALING_H

#include "prepareddata.h"

class AlgoAnnealing
{
public:
    AlgoAnnealing(const prepared::DataStatic ads);

    prepared::DataDynamic find();



private:
    prepared::DataDynamic find(double &progress);

private:
    prepared::DataStatic ds;
};

#endif // ALGOANNEALING_H
