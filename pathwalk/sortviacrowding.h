#ifndef SORTVIACROWDING_H
#define SORTVIACROWDING_H

#include "prepareddata.h"

#include <QVector>


using SubPath = QVector<int>;
using Path = QVector<SubPath>;

// проверяет трассы на скученность в конкретных местах и при наличии таких скученностей группирует их сортировкой
// возвращает скученности (если есть) иначе пустое
Path pathViaCrowding(const QVector<prepared::Trac> &tracs);

QVector<prepared::Trac> applyPath(const Path &path, const QVector<prepared::Trac> &tracs);


#endif // SORTVIACROWDING_H
