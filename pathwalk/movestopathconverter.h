#ifndef MOVESTOPATHCONVERTER_H
#define MOVESTOPATHCONVERTER_H

#include <QVector>
#include <QPair>

#include "prepareddata.h"

struct ShipMove
{
    prepared::Line line;
    bool isP1Start{ false };

    QPoint first() const { return isP1Start ? line.p1() : line.p2(); }
    QPoint second() const { return isP1Start ? line.p2() : line.p1(); }
};

using ShipMovesVector = QVector<ShipMove>;


/**
 * @brief Конвертер корректных движений судов ShipMovesVector в корректный путь
 *
 * Исходные данные ShipMovesVector содержат только выбор дальнейних точек судов, без привязки ко времени
 * Данный класс конвертирует подобные записи в реальный Path, рассчитывая минимальное время
 *
 * Стоит отметить:
 *   от handler ожидается, что каждый путь будет указан 2 раза, а число датчиков на корабле не будет отрицательным
 *   от shooter ожидается, что каждый путь будет указан 1 раз
 */
class MovesToPathConverter
{
public:
    MovesToPathConverter(const prepared::DataStatic &ads);

    struct PathAndCost {
        QString path;
        qlonglong cost{ -1 };

        bool isValid() const { return cost > 0; }
    };

    void setShips(const prepared::Handler &ship1, const prepared::Shooter &ship2){ handler = ship1; shooter = ship2; }
    //PathAndCost createRawPath(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec);
    PathAndCost createPath(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec);

private:
    void clear();

private:
    QMap<prepared::Line, prepared::Trac> tracMap;
    prepared::DataStatic ds;
    prepared::Shooter shooter;
    prepared::Handler handler;
    QMap<prepared::Line, char> lineStateMap;
    QMap<prepared::Line, int> lineStateChangedMap;
};

#endif // MOVESTOPATHCONVERTER_H
