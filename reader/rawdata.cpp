#include "rawdata.h"

#include <QDebug>

void raw::Data::printToDebug() const
{
    qDebug() << "-- begin output current data";
    qDebug() << formatTrac;
    for (int i = 0; i < trac.size(); ++i) {
        const auto & n{ trac.at(i) };
        qDebug() << n.x0 << n.y0 << n.x1 << n.y1 << n.layoutStep;
    }
    qDebug() << formatSeparator;

    qDebug() << formatShip;
    for (int i = 0; i < ship.size(); ++i) {
        const auto & n{ ship.at(i) };
        if (n.type == Ship::Type::shooter)
            qDebug().noquote() << n.typeToQChar() << n.name << n.speed;
        else
            qDebug().noquote() << n.typeToQChar() << n.name << n.speed << n.maxSensorCount;
    }
    qDebug() << formatSeparator;

    qDebug() << formatMone;
    qDebug() << sensorMone.money();
    for (int i = 0; i < shipMone.size(); ++i) {
        const auto & n{ shipMone.at(i) };
        qDebug().noquote() << n.name << n.money;
    }
    qDebug() << formatSeparator;

    qDebug() << formatIcee;
    for (int i = 0; i < icee.size(); ++i) {
        const auto & n{ icee.at(i) };
        qDebug() << n.trackNumber << n.close << n.open;
    }
    qDebug() << formatSeparator;

    qDebug() << formatPath;
    for (int i = 0; i < path.size(); ++i) {
        const auto & n{ path.at(i) };
        {
            auto d { qDebug().noquote() };
            d << Ship::typeToQChar(n.type) << n.name << n.size;
            if (n.size != n.path.size())
                d << "but real size is" << n.path.size();
        }

        for (int k = 0; k < n.path.size(); ++k) {
            const auto & nn{ n.path.at(k) };
            qDebug() << nn.x << nn.y << nn.timeH << nn.activity;
        }
    }
    qDebug() << formatSeparator;
    qDebug() << "-- end output current data";
}
