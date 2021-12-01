#ifndef ALGODUMMY_H
#define ALGODUMMY_H

#include "prepareddata.h"

class AlgoDummy
{
public:
    AlgoDummy(const prepared::DataStatic ads);
    prepared::DataDynamic find();

private:
    prepared::DataStatic ds;
};

#endif // ALGODUMMY_H
