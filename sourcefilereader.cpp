#include "sourcefilereader.h"

#include <QDebug>
#include <QDir>
#include <QVector>

// Добавим для удобства структурки входных данных
namespace sourceData {

struct Trac
{
    double x0, y0, x1, y1, layoutStep;
};

struct Ship
{
    double speed;
    QString name;
    int maxSensorCount;
    enum class Type : bool {
        shooter,
        handler
    } type;
    char unused[7];

    static QChar typeToQChar(Ship::Type type) { return type == Type::shooter ? 'S' : 'H'; }
    QChar typeToQChar() const { return typeToQChar(type); }
};

struct SensorMone
{
    double money{ -1. };
};

struct ShipMone
{
    QString name;
    double money;
};

struct Icee
{
    int trackNumber;
    double open, close;
};

struct PathDot
{
    double x, y, timeH;
    int activity;
};

struct Path
{
    int size;
    QString name;
    QVector<PathDot> path;
    Ship::Type type;
    char unused[3];
};

struct Data
{
    SensorMone sensorMone;
    QVector<Trac> trac;
    QVector<Ship> ship;
    QVector<ShipMone> shipMone;
    QVector<Icee> icee;
    QVector<Path> path;
    char unused[4];
};

} // end sourceData namespace
using namespace sourceData;

SourceFileReader::SourceFileReader(const QString &filename)
{
    if (!filename.isEmpty())
        readSourceFile(filename);
}

void SourceFileReader::clear()
{
    delete data;
    data = nullptr;
}

SourceFileReader::~SourceFileReader()
{
    clear();
}

void SourceFileReader::readSourceFile(QString filename)
{
    clear();
    data = new Data;
    Q_ASSERT(!in.sourceFile.isOpen());
    filename = QFileInfo(filename).absoluteFilePath();
    in.sourceFile.setFileName(filename);
    if (!in.sourceFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qInfo() << "Невозможно открыть файл для чтения по пути:" << filename;
        return;
    }

    while (!in.fileAtEnd()) {
        in.readLine();
        if (in.testAndSetFormat(formatTrac))
            readTrac();
        else if (in.testAndSetFormat(formatShip))
            readShip();
        else if (in.testAndSetFormat(formatMone))
            readMone();
        else if (in.testAndSetFormat(formatIcee))
            readIcee();
        else if (in.testAndSetFormat(formatPath))
            readPath();
        else if (in.currentLine.isEmpty()) {
            warningEmptyString();
        }
        else if (in.lineIsBlockEnd())
            qInfo().noquote() << QString{"Строка %1. Ожидалось ключевое слово, но был встречен лишний символ конца блока '%2'. "
                                         "Данная строка не будет учитана"}.arg(in.strLineNumber()).arg(in.currentLine);
        else {
            qInfo().noquote() << QString{"Строка %1. Ожидалось ключевое слово, но была встречена строка '%2'. "
                                         "Данная строка не будет учитана"}.arg(in.strLineNumber()).arg(in.currentLine);
        }
    }
    in.sourceFile.close();
}

void SourceFileReader::readTrac()
{
    while (true) {
        if (in.fileAtEnd()) {
            warningUnexpectedEndOfFile();
            break;
        }

        in.readLine();

        if (in.lineIsBlockEnd()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 5) {
            warningArgCount();
            continue;
        }

        Trac trac;
        bool cont{ false };
        if (!convertWithWarnings(trac.x0, 0))
            cont = true;
        if (!convertWithWarnings(trac.y0, 1))
            cont = true;
        if (!convertWithWarnings(trac.x1, 2))
            cont = true;
        if (!convertWithWarnings(trac.y1, 3))
            cont = true;
        if (!convertWithWarnings(trac.layoutStep, 4))
            cont = true;
        if (cont)
            continue;

        data->trac.append(trac);
    }
}

void SourceFileReader::readShip()
{
    while (true) {
        if (in.fileAtEnd()) {
            warningUnexpectedEndOfFile();
            break;
        }

        in.readLine();

        if (in.lineIsBlockEnd()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 3 && in.argSize() != 4) {
            warningArgCount();
            continue;
        }

        Ship ship;

        if (in.argRefAt(0) == 'S')
            ship.type = Ship::Type::shooter;
        else if (in.argRefAt(0) == 'H')
            ship.type = Ship::Type::handler;
        else {
            qInfo().noquote() << QString("Строка %1. Ошибка при чтении в %2: корабль имеет некорректный тип '%3'. "
                                         "Данная строка не будет учитана. Исходный текст строки: '%4'")
                                 .arg(in.strLineNumber()).arg(in.currentFormatString).arg(in.argRefAt(0))
                                 .arg(in.currentLine);
            continue;
        }
        ship.name = in.argAt(1);
        if (!convertWithWarnings(ship.speed, 2))
            continue;
        const bool hViaType{ ship.type == Ship::Type::handler };
        const bool hViaArgC{ in.argSize() == 4 };
        if ((hViaType || hViaArgC) != (hViaType && hViaArgC)) {
            qInfo().noquote() << QString("Строка %1. Ошибка при чтении в %2: корабль имеет несоотвествие данных: "
                                         "%3 с %4 входными аргументами. Данная строка не будет учитана. "
                                         "Исходный текст строки: '%5'")
                                 .arg(in.strLineNumber()).arg(in.currentFormatString).arg(hViaType ? "укладчик" : "шутер")
                                 .arg(in.argSize());
            continue;
        }

        if (hViaArgC && !convertWithWarnings(ship.maxSensorCount, 3))
            continue;

        data->ship.append(ship);
    }
}

void SourceFileReader::readMone()
{
    while (true) {
        if (in.fileAtEnd()) {
            warningUnexpectedEndOfFile();
            return;
        }

        in.readLine();

        if (in.lineIsBlockEnd()) {
            return;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 1) {
            warningArgCount();
            continue;
        }

        double money;
        if (!convertWithWarnings(money, 0))
            return;
        data->sensorMone.money = money;
        break;
    }

    while (true) {
        if (in.fileAtEnd()) {
            warningUnexpectedEndOfFile();
            break;
        }

        in.readLine();

        if (in.lineIsBlockEnd()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 2) {
            warningArgCount();
            continue;
        }

        ShipMone shipMone;
        shipMone.name = in.argAt(0);

        if (!convertWithWarnings(shipMone.money, 1))
            continue;

        data->shipMone.append(shipMone);
    }
}

void SourceFileReader::readIcee()
{
    while (true) {
        if (in.fileAtEnd()) {
            warningUnexpectedEndOfFile();
            break;
        }

        in.readLine();

        if (in.lineIsBlockEnd()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 3) {
            warningArgCount();
            continue;
        }

        Icee icee;
        bool cont{ false };
        if (!convertWithWarnings(icee.trackNumber, 0))
            cont = true;
        if (!convertWithWarnings(icee.close, 1))
            cont = true;
        if (!convertWithWarnings(icee.open, 2))
            cont = true;
        if (cont)
            continue;

        data->icee.append(icee);
    }
}

void SourceFileReader::readPath()
{
    while (true) {
        if (in.fileAtEnd()) {
            warningUnexpectedEndOfFile();
            break;
        }

        in.readLine();

        if (in.lineIsBlockEnd()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 3) {
            warningArgCount();
            continue;
        }

        Path shipPath;
        if (in.argRefAt(0) == 'S')
            shipPath.type = Ship::Type::shooter;
        else if (in.argRefAt(0) == 'H')
            shipPath.type = Ship::Type::handler;
        else {
            qInfo().noquote() << QString("Строка %1. Ошибка при чтении в %2: корабль имеет некорректный тип '%3'. "
                                         "Данный путь не будет учитан. Исходный текст строки: '%4'")
                                 .arg(in.strLineNumber()).arg(in.currentFormatString).arg(in.argRefAt(0))
                                 .arg(in.currentLine);
            continue;
        }
        shipPath.name = in.argAt(1);
        if (!convertWithWarnings(shipPath.size, 2))
            continue;

        shipPath.path.reserve(shipPath.size);

        // чтение точек для пути корабля
        for (int i = 0; i < shipPath.size; ++i) {
            if (in.fileAtEnd()) {
                qInfo().noquote() << QString("Строка %1. Ошибка при чтении точки в %2: Встречен конец файла, хотя ожидалось "
                                             "получить ещё %3 строк для корабля %4")
                                     .arg(in.strLineNumber()).arg(in.currentFormatString).arg(shipPath.size - i)
                                     .arg(shipPath.name);
                in.reverseReadLine();
                break;
            }
            in.readLine();

            if (in.lineIsBlockEnd()) {
                qInfo().noquote() << QString("Строка %1. Ошибка при чтении точки в %2: Встречен конец блока, хотя ожидалось "
                                             "получить ещё %3 строк для корабля %4")
                                     .arg(in.strLineNumber()).arg(in.currentFormatString).arg(shipPath.size - i)
                                     .arg(shipPath.name);
                in.reverseReadLine();
                break;
            }

            if (in.lineIsEmpty()) {
                // мы не должны учитывать пустые строки
                --i;
                warningEmptyString();
                continue;
            }

            if (in.argSize() != 4) {
                warningArgCount();
                continue;
            }

            PathDot pathDot;
            bool cont{ false };
            if (!convertWithWarnings(pathDot.x, 0))
                cont = true;
            if (!convertWithWarnings(pathDot.y, 1))
                cont = true;
            if (!convertWithWarnings(pathDot.timeH, 2))
                cont = true;
            if (!convertWithWarnings(pathDot.activity, 3))
                cont = true;
            if (cont)
                continue;

            shipPath.path.append(pathDot);
        }

        data->path.append(shipPath);
    }
}

void SourceFileReader::warningArgCount()
{
    qInfo().noquote() << QString("Строка %1. Ошибка при чтении в %2: строка имеет некорректное ожидаемому число "
                                 "аргументов (получено %3). Данная строка не будет учитана. Исходный "
                                 "текст строки: '%4'")
                         .arg(in.strLineNumber()).arg(in.currentFormatString).arg(in.argSize()).arg(in.currentLine);
}


void SourceFileReader::warningArgConvertToDouble(int badArgNumber)
{
    qInfo().noquote() << QString("Строка %1. Ошибка при чтении в %2: строка имеет неконвертируемую в число запись. "
                                 "Данная строка не будет учитана. Аргумент по номеру %3: '%4'. Исходный "
                                 "текст строки: '%5'")
                         .arg(in.strLineNumber()).arg(in.currentFormatString).arg(badArgNumber)
                         .arg(in.argRefAt(badArgNumber)).arg(in.currentLine);
}

void SourceFileReader::warningUnexpectedEndOfFile()
{
    qInfo().noquote() << QString("Строка в конце файла (примерно %1). Ошибка при чтении в %2: обнаружен конец файла, но текущий блок не был закрыт.")
                         .arg(in.lineNumber()).arg(in.currentFormatString);
}

void SourceFileReader::warningEmptyString()
{
    // не будем писать что-то такое...
    //qInfo().noquote()  << QString("Строка %1. Предупреждение. Встречена пустая строка в файле исходных данных")
    //                      .arg(in.strLineNumber());
}

void SourceFileReader::print() const
{
    qDebug() << "-- current data";
    if (!data) {
        qDebug() << "Source data is empty. File not read.";
        return;
    }

    qDebug() << formatTrac;
    for (int i = 0; i < data->trac.size(); ++i) {
        const auto & n{ data->trac.at(i) };
        qDebug() << n.x0 << n.y0 << n.x1 << n.y1 << n.layoutStep;
    }
    qDebug() << formatSeparator;

    qDebug() << formatShip;
    for (int i = 0; i < data->ship.size(); ++i) {
        const auto & n{ data->ship.at(i) };
        if (n.type == Ship::Type::shooter)
            qDebug().noquote() << n.typeToQChar() << n.name << n.speed;
        else
            qDebug().noquote() << n.typeToQChar() << n.name << n.speed << n.maxSensorCount;
    }
    qDebug() << formatSeparator;

    qDebug() << formatMone;
    qDebug() << data->sensorMone.money;
    for (int i = 0; i < data->shipMone.size(); ++i) {
        const auto & n{ data->shipMone.at(i) };
        qDebug().noquote() << n.name << n.money;
    }
    qDebug() << formatSeparator;

    qDebug() << formatIcee;
    for (int i = 0; i < data->icee.size(); ++i) {
        const auto & n{ data->icee.at(i) };
        qDebug() << n.trackNumber << n.close << n.open;
    }
    qDebug() << formatSeparator;

    qDebug() << formatPath;
    for (int i = 0; i < data->path.size(); ++i) {
        const auto & n{ data->path.at(i) };
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
}
