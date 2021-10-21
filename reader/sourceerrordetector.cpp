#include "sourceerrordetector.h"

#include "rawdata.h"

#include <QDebug>
#include <QSet>

using namespace raw;

SourceErrorDetector::SourceErrorDetector(const raw::Data &adata)
{
    setDataAndDetectErrors(adata);
}

SourceErrorDetector::~SourceErrorDetector()
{
    clear();
}

void SourceErrorDetector::setDataAndDetectErrors(const raw::Data &adata)
{
    clear();
    data = new Data(adata);

    detectErrors();
}

void SourceErrorDetector::clear()
{
    delete data;
    data = nullptr;
}

void SourceErrorDetector::detectErrors()
{
    Q_ASSERT(data);
    errorsWithLimits();
    errorsNoRequiredData();
    errorsWithShipNames();
    errorsWithTracIntersects();
}

void SourceErrorDetector::errorsWithLimits()
{
    errorsWithLimitsTrac();
    errorsWithLimitsShip();
    errorsWithLimitsMone();
    errorsWithLimitsIcee();
    errorsWithLimitsPath();
}

void SourceErrorDetector::errorsWithLimitsTrac()
{
    const auto & tracs{ data->trac };
    for (int i = 0; i < tracs.size(); ++i) {
        const auto & trac{ tracs.at(i) };
        if (trac.layoutStep <= 0) {
            qWarning()<< "Задан некорректный (неположительный) шаг раскладки датчиков в" << formatTrac
                      << "для пути" << trac.x0 << trac.y0 << trac.x1 << trac.y1 << "("
                      << trac.layoutStep << ")";
        }
        if (trac.x0 == trac.x1 && trac.y0 == trac.y1) {
            qInfo() << "Предупреждение. Встречена трасса нулевой длины. Для её прохода будет достаточно одного датчика";
        }
        // Возможно добавить проверку на нулевой путь, но это просто как предупреждение
    }
}

void SourceErrorDetector::errorsWithLimitsShip()
{
    const auto & ship{ data->ship };
    for (int i = 0; i < ship.size(); ++i) {
        if (ship.at(i).speed <= 0) {
            qWarning()<< "Задана некорректная (неположительная) скорость в" << formatShip
                      << "корабля" << ship.at(i).typeToQChar() << ship.at(i).name << "("
                      << ship.at(i).speed << ")";
        }
        if (ship.at(i).type == Ship::Type::handler) {
            if (ship.at(i).maxSensorCount <= 0) {
                qWarning()<< "Задано некорректное (неположительное) максимальное число датчиков в" << formatShip
                          << "на укладчике" << ship.at(i).typeToQChar() << ship.at(i).name << "("
                          << ship.at(i).maxSensorCount << ")";
            }
            if (ship.at(i).maxSensorCount > 20000) {
                qInfo()<< "Предупреждение. По условию на укладчик можно разместить не более 20000 сенсоров, но на"
                       << ship.at(i).typeToQChar() << ship.at(i).name << " это число составляет ("
                       << ship.at(i).maxSensorCount << ")";
            }
        }
    }
}

void SourceErrorDetector::errorsWithLimitsMone()
{
    if (data->sensorMone.valid() && data->sensorMone.money() <= 0) {
        qWarning()<< "Задана некорректная (неположительная) стоимость аренды датчиков в" << formatMone << "("
                  << data->sensorMone.money() << ")";
    }

    const auto & mone{ data->shipMone };
    for (int i = 0; i < mone.size(); ++i) {
        if (mone.at(i).money <= 0) {
            qWarning()<< "Задана некорректная (неположительная) стоимость аренды судна в" << formatMone
                      << Ship::typeToQChar(mone.at(i).type) << mone.at(i).name << "(" << mone.at(i).money << ")";
        }
    }
}

void SourceErrorDetector::errorsWithLimitsIcee()
{
    const auto & icee{ data->icee };
    for (int i = 0; i < icee.size(); ++i) {
        if (icee.at(i).close >= icee.at(i).open) {
            qWarning()<< "Задано некорректное время закрытия и открытия трасс в" << formatIcee
                      << "(закрытие должно происходить раньше, чем открытие) для номера"
                      << icee.at(i).trackNumber << "закрытие:" << icee.at(i).close << "открытие:" << icee.at(i).open;
        }
        if (icee.at(i).open < 0) {
            qInfo()<< "Предупреждение. Время закрытия трассы в" << formatIcee  << "ожидается неотрицательным для номера"
                   << icee.at(i).trackNumber << "закрытие:" << icee.at(i).close << "открытие:" << icee.at(i).open;
        }
        if (icee.at(i).trackNumber < 1 || icee.at(i).trackNumber > data->trac.size()) {
            qWarning()<< "В" << formatIcee << "задан номер трассы, которого нет в" << formatTrac
                      << "(" << icee.at(i).trackNumber << "). Данные трассы:"
                      << icee.at(i).trackNumber << icee.at(i).close << icee.at(i).open;
        }
    }
}

void SourceErrorDetector::errorsWithLimitsPath()
{
    QSet<int> opsH{ 0, 1, 2, 3 };
    QSet<int> opsS{ 0, 1, 4 };
    const auto & path{ data->path };
    for (int i = 0; i < path.size(); ++i) {
        if (path.at(i).size <= 0) {
            qWarning()<< "Задано некорректное (неположительное) число записей для пути" << Ship::typeToQChar(path.at(i).type)
                      << path.at(i).name << "(" << path.at(i).size << ")";
        }
        const auto & pathdots{ path.at(i).path };
        if (path.at(i).type == Ship::Type::shooter) {
            for (int k = 0; k < pathdots.size(); ++k) {
                if (pathdots.at(k).timeH < 0) {
                    qWarning()<< "Задано некорректное время операции действия в пути корабля" << Ship::typeToQChar(path.at(i).type)
                              << path.at(i).name << "(" << pathdots.at(k).timeH << ")";
                }
                if (!opsS.contains(pathdots.at(k).activity)) {
                    qWarning()<< "Задана некорректная операция действия в пути шутера" << Ship::typeToQChar(path.at(i).type)
                              << path.at(i).name << "(" << pathdots.at(k).activity << ")";
                }
            }
        }
        else {
            for (int k = 0; k < pathdots.size(); ++k) {
                if (pathdots.at(k).timeH < 0) {
                    qWarning()<< "Задано некорректное время операции действия в пути корабля" << Ship::typeToQChar(path.at(i).type)
                              << path.at(i).name << "(" << pathdots.at(k).timeH << ")";
                }
                if (!opsH.contains(pathdots.at(k).activity)) {
                    qWarning()<< "Задана некорректная операция действия в пути обработчика" << Ship::typeToQChar(path.at(i).type)
                              << path.at(i).name << "(" << pathdots.at(k).activity << ")";
                }
            }
        }
    }
}

void SourceErrorDetector::errorsNoRequiredData()
{
    errorsNoTracs();
    errorsNoShips();
    errorsNoMone();
}

void SourceErrorDetector::errorsNoTracs()
{
    if (data->trac.isEmpty()) {
        qWarning().noquote() << "В" << formatTrac << "отсутствуют трассы. Нечего делать";
    }
}

void SourceErrorDetector::errorsNoShips()
{
    bool hasH{ false }, hasS{ false };
    const auto shipVector{ data->ship };
    for (int i = 0; i < shipVector.size(); ++i) {
        hasH |= shipVector.at(i).type == Ship::Type::handler;
        hasS |= shipVector.at(i).type == Ship::Type::shooter;

        if (hasH && hasS)
            return;
    }

    QStringList noShips;
    if (!hasH)
        noShips << "укладчик";
    if (!hasS)
        noShips << "шутер";

    qWarning().noquote() << "В" << formatShip << "отсутствуют корабли типа:" << noShips.join(" и ");
}

void SourceErrorDetector::errorsNoMone()
{
    if (!data->sensorMone.valid()) {
        qWarning().noquote() << "В" << formatMone << "нет информации о стоимости датчиков";
    }
}

void SourceErrorDetector::errorsWithShipNames()
{
    errorsWithShipNamesUnic();
    errorsWithShipNamesUnknown();
}

namespace {

QString fullShipName(Ship::Type type, const QString &name)
{
    return Ship::typeToQChar(type) + name;
}

template<typename T>
void errorsWithShipNamesUnicTemplate(const QVector<T> &t, Ship::Type T::* type, QString T::* name, const char *format)
{
    QSet<QString> names;

    for (int i = 0; i < t.size(); ++i) {
        const auto & ship{ t.at(i) };
        const auto fullname{ fullShipName(ship.*type, ship.*name) };
        if (!names.contains(fullname))
            names.insert(fullname);
        else {
            qWarning()<< "В" << format << "встречен корабль с именем, которое уже занято другим кораблём данного типа ("
                      << ship.*type << ship.*name << ")";
        }
    }
}

} // end anonymous namespace

void SourceErrorDetector::errorsWithShipNamesUnic()
{
    errorsWithShipNamesUnicTemplate(data->ship, &Ship::type, &Ship::name, formatShip);
    errorsWithShipNamesUnicTemplate(data->shipMone, &ShipMone::type, &ShipMone::name, formatMone);
    errorsWithShipNamesUnicTemplate(data->path, &Path::type, &Path::name, formatPath);
}

namespace {

template<typename T>
void errorsWithShipNamesUnknownTemplate(const QVector<raw::Ship> &ships, const QVector<T> &t, Ship::Type T::* type,
                                        QString T::* name, const char *format, bool warningNone=false)
{
    QSet<QString> namesShip, namesT;

    for (int i = 0; i < ships.size(); ++i) {
        namesShip.insert(fullShipName(ships.at(i).type, ships.at(i).name));
    }

    for (int i = 0; i < t.size(); ++i) {
        namesT.insert(fullShipName(t.at(i).*type, t.at(i).*name));
    }

    const auto extraT{ namesT - namesShip };
    if (!extraT.isEmpty()) {
        QStringList s;
        for (const auto & ex : extraT)
            s << ex.mid(0, 1) + ' ' + ex.mid(1);
        qWarning().noquote() << "В" << format << "встречены неописанные в" << formatShip << "корабли:" << s.join(", ");
    }

    if (warningNone) {
        const auto noneT{ namesShip - namesT };
        if (!noneT.isEmpty()) {
            QStringList s;
            for (const auto & none : noneT)
                s << none.mid(0, 1) + ' ' + none.mid(1);
            qWarning().noquote() << "В" << format << "пропущены описанные в" << formatShip << "корабли:" << s.join(", ");
        }
    }
}

} // end anonymous namespace

void SourceErrorDetector::errorsWithShipNamesUnknown()
{
    errorsWithShipNamesUnknownTemplate(data->ship, data->shipMone, &ShipMone::type, &ShipMone::name, formatMone, true);
    errorsWithShipNamesUnknownTemplate(data->ship, data->path, &Path::type, &Path::name, formatPath);
}

void SourceErrorDetector::errorsWithTracIntersects()
{
    // оказывается, что это не ошибки, поэтому не стоит что-либо делать
    return;
    Q_UNREACHABLE();

    QMap<int, QVector<QPair<int, int> > > map;

    for (int i = 0; i < data->icee.size(); ++i) {
        const auto & icee{ data->icee.at(i) };
        auto tracs{ map.value(icee.trackNumber) };
        tracs.append(qMakePair(icee.close, icee.open));
        map.insert(icee.trackNumber, tracs);
    }

    const auto & keys{ map.keys() };
    for (int i = 0; i < keys.size(); ++i) {
        auto tracs{ map.value(keys.at(i)) };
        std::sort(tracs.begin(), tracs.end());

        for (int k = 1; k < tracs.size(); ++k) {
            const auto & current{ tracs.at(k-1) };
            const auto & next{ tracs.at(k) };

            if (next.first <= current.second) {
                // уже ясно, что они пересекаются
                if (next.second <= current.second) {
                    qWarning().noquote() << "В" << formatIcee << "для пути" << keys.at(i) << "ограничение"
                                         << "(" << current.first << current.second << ") полностью поглощает ограничение"
                                         << "(" << next.first << next.second << ")";
                }
                else {
                    qWarning().noquote() << "В" << formatIcee << "для пути" << keys.at(i) << "ограничение"
                                         << "(" << current.first << current.second << ") пересекается с ограничением"
                                         << "(" << next.first << next.second << ")";
                }
            }
        }
    }
}
