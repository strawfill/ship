#ifndef MOVESTOPATHCONVERTER_H
#define MOVESTOPATHCONVERTER_H

#include <QVector>
#include <QPair>

#include "prepareddata.h"

struct ShipMove
{
    ShipMove() {}
    ShipMove(short tn, bool p1s) : m_tracNum(tn), m_isP1Start(p1s) {}

    QPoint first(const QVector<prepared::Trac> &tracs) const { return m_isP1Start ? tracs.at(m_tracNum).p1() : tracs.at(m_tracNum).p2(); }
    QPoint second(const QVector<prepared::Trac> &tracs) const { return m_isP1Start ? tracs.at(m_tracNum).p2() : tracs.at(m_tracNum).p1(); }

    short trac() const { return m_tracNum; }
    bool isStartP1() const { return m_isP1Start; }
    void setStartPoint(bool startFromP1) { m_isP1Start = startFromP1; }
    void reverseStartPoint() { m_isP1Start = !m_isP1Start; }

private:
    short m_tracNum{ 0 };
    bool m_isP1Start{ false };
};

using ShipMovesVector = QVector<ShipMove>;
struct ShipMoveData
{
    ShipMovesVector handler;
    ShipMovesVector shooter;

    void reserve(int size) { handler.reserve(2*size); shooter.reserve(size); }
};


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


    void setShips(const prepared::Handler &ship1, const prepared::Shooter &ship2);

    PathAndTime createPath(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec);
    int calculateHours(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec);
    qlonglong calculateCost(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec);

    prepared::DataDynamic createDD(const ShipMovesVector &handlerVec, const ShipMovesVector &shooterVec)
    { return createDD(createPath(handlerVec, shooterVec)); }
    prepared::DataDynamic createDD(const PathAndTime &path);

private:
    std::vector<char> lineState;
    std::vector<int>lineStateChanged;
    PathAndTime pat;

    const prepared::DataStatic &ds;
    prepared::Shooter shooter;
    prepared::Handler handler;
};

#endif // MOVESTOPATHCONVERTER_H
