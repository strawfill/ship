#ifndef MOVESTOPATHCONVERTER_H
#define MOVESTOPATHCONVERTER_H

#include <QVector>
#include <QPair>

#include "prepareddata.h"

struct ShipMove
{
    short tracNum{ 0 };
    bool isP1Start{ false };

    QPoint first(const QVector<prepared::Trac> &tracs) const { return isP1Start ? tracs.at(tracNum).p1() : tracs.at(tracNum).p2(); }
    QPoint second(const QVector<prepared::Trac> &tracs) const { return isP1Start ? tracs.at(tracNum).p2() : tracs.at(tracNum).p1(); }
};

using ShipMovesVector = QVector<ShipMove>;


/**
 * @brief Конвертер корректных движений судов ShipMovesVector в корректный путь
 *
 * Исходные данные ShipMovesVector содержат только выбор дальнейних точек судов, без привязки ко времени
 * Данный класс конвертирует подобные записи в реальный Path, рассчитывая минимальное время
 *
 * О calculateHours и createPath стоит отметить:
 *   от handler ожидается, что каждый путь будет указан 2 раза
 *   от shooter ожидается, что каждый путь будет указан 1 раз
 *   не проверяется отрицательное число датчиков на судне, нужно вызвать метод handlerCanPassIt
 */
class MovesToPathConverter
{
public:
    MovesToPathConverter(const prepared::DataStatic &ads);

    struct PathAndTime {
        prepared::Path handlerPath;
        prepared::Path shooterPath;
        int time{ -1 };

        bool isValid() const { return time > 0; }
    };
    struct StringPathAndCost {
        QString path;
        qlonglong cost{ -1 };

        bool isValid() const { return cost > 0; }
    };

    // проверить, не будет ли отрицательное число датчиков на судне согласно заданному маршруту
    // на входе ожидаются номера трасс, строго по два включения номера для каждой трассы
    bool handlerCanPassIt(const std::vector<int> handlerVec) const;

    void setShips(const prepared::Handler &ship1, const prepared::Shooter &ship2);

    PathAndTime createPath(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec);
    int calculateHours(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec);

    prepared::DataDynamic createDD(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
    { return createDD(createPath(handlerVec, shooterVec)); }
    prepared::DataDynamic createDD(const PathAndTime &path);

private:
    std::vector<char> lineState;
    std::vector<int>lineStateChanged;
    PathAndTime pat;

    prepared::DataStatic ds;
    prepared::Shooter shooter;
    prepared::Handler handler;
};

#endif // MOVESTOPATHCONVERTER_H
