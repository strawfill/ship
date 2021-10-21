#ifndef RAWDATA_H
#define RAWDATA_H

#include <QtGlobal>
#include <QString>
#include <QVector>
#include <QDebug>

// структуры данных, напрямую считываемые из файла
// нужны для первичной обработки информации, когда создавать готовые структуры данных ещё рано
namespace raw {

static constexpr const char* formatTrac{ "TRAC" };
static constexpr const char* formatShip{ "SHIP" };
static constexpr const char* formatMone{ "MONE" };
static constexpr const char* formatIcee{ "ICEE" };
static constexpr const char* formatPath{ "PATH" };
static constexpr const char formatSeparator{ '/' };

struct Trac
{
    int x0, y0, x1, y1, layoutStep;
};

struct Ship
{
    int speed;
    QString name;
    int maxSensorCount{ 0 };
    enum class Type : bool {
        shooter,
        handler
    } type;

    static QChar typeToQChar(Ship::Type type) { return type == Type::shooter ? 'S' : 'H'; }
    QChar typeToQChar() const { return typeToQChar(type); }
};

struct SensorMone
{
    // чтобы проверить, что число задано
    int money() const { return mon; }
    bool valid() const { return isValid; }
    void setMoney(int amoney) { mon = amoney; isValid = true; }
private:
    int mon{ 0 };
    bool isValid{ false };
};

struct ShipMone
{
    QString name;
    int money;
    Ship::Type type;
};

struct Icee
{
    int trackNumber, open, close;
};

struct PathDot
{
    int x, y, timeH, activity;
};

struct Path
{
    int size;
    QString name;
    QVector<PathDot> path;
    Ship::Type type;
};

struct Data
{
    QVector<Trac> trac;
    QVector<Ship> ship;
    SensorMone sensorMone;
    QVector<ShipMone> shipMone;
    QVector<Icee> icee;
    QVector<Path> path;

    void printToDebug() const;
};

} // end namespace Raw


inline QDebug operator<<(QDebug debug, const raw::Ship::Type type)
{
    return debug << raw::Ship::typeToQChar(type);
}



#endif // RAWDATA_H
