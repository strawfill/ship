#ifndef ALGOBRUTEFORCE_H
#define ALGOBRUTEFORCE_H

#include "prepareddata.h"

class AlgoBruteForce
{
public:
    AlgoBruteForce(const prepared::DataStatic ads);

    prepared::DataDynamic find();



private:
    prepared::DataDynamic find(double &progress);

private:
    prepared::DataStatic ds;
};

#endif // ALGOBRUTEFORCE_H
