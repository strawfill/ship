#include "sourcefilereader.h"

#include "rawdata.h"
#include "debugcatcher.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QVector>

using namespace raw;

namespace {
QElapsedTimer timer{ [](){ QElapsedTimer t; t.start(); return t; }() };

void resetProcessEventsTimer()
{
    timer.restart();
}

void maybeProcessEvents()
{
    if (timer.elapsed() > 20) {
        timer.restart();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }
}
} // end anomymous namespace

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

Data SourceFileReader::dat() const
{
    if (data)
        return *data;
    return {};
}

const Data &SourceFileReader::constDat() const
{
    if (data)
        return *data;

    const static Data empty;
    return empty;
}

SourceFileReader::~SourceFileReader()
{
    clear();
}

void SourceFileReader::readSourceFile(QString filename)
{
    resetProcessEventsTimer();
    DebugCatcher::instance()->clearWaringsCount();

    clear();
    data = new Data;
    Q_ASSERT(!in.sourceFile.isOpen());
    filename = QFileInfo(filename).absoluteFilePath();
    in.sourceFile.setFileName(filename);
    if (!QFileInfo::exists(filename)) {
        qWarning() << "Файл не существует. Заданный путь:" << filename;
        return;
    }
    if (!in.sourceFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
        qWarning() << "Невозможно открыть файл для чтения по пути:" << filename;
        return;
    }

    bool wasTrac{ false }, wasShip{ false }, wasMone{ false }, wasIcee{ false }, wasPath{ false };

    while (!in.fileAtEnd()) {
        in.readLine();
        if (in.testAndSetFormat(formatTrac)) {
            if (wasTrac) {
                warningExtraBlock(formatTrac);
                readFictive();
            }
            else {
                readTrac();
                wasTrac = true;
            }
        }
        else if (in.testAndSetFormat(formatShip)) {
            if (wasShip) {
                warningExtraBlock(formatShip);
                readFictive();
            }
            else {
                readShip();
                wasShip = true;
            }
        }
        else if (in.testAndSetFormat(formatMone)) {
            if (wasMone) {
                warningExtraBlock(formatMone);
                readFictive();
            }
            else {
                readMone();
                wasMone = true;
            }
        }
        else if (in.testAndSetFormat(formatIcee)) {
            if (wasIcee) {
                warningExtraBlock(formatIcee);
                readFictive();
            }
            else {
                readIcee();
                wasIcee = true;
            }
        }
        else if (in.testAndSetFormat(formatPath)) {
            if (wasPath) {
                warningExtraBlock(formatPath);
                readFictive();
            }
            else {
                readPath();
                wasPath = true;
            }
        }
        else if (in.currentLine.isEmpty()) {
            warningEmptyString();
        }
        else if (in.lineIsBlockEnd())
            qWarning().noquote() << QString{"Строка %1. Ожидалось ключевое слово, но был встречен лишний символ конца блока '%2'. "
                                            "Данная строка не будет учтена"}.arg(in.strLineNumber()).arg(in.currentLine);
        else {
            qWarning().noquote() << QString{"Строка %1. Ожидалось ключевое слово, но была встречена строка '%2'. "
                                            "Данная строка не будет учтена"}.arg(in.strLineNumber()).arg(in.currentLine);
        }
        enum { max_error_count = 100 };
        if (DebugCatcher::instance()->warningsCount() > max_error_count) {
            qWarning().noquote() << QString{"Файл содержит не менее %1 ошибок. Дальнейший разбор содержимого "
                                            "остановлен "}.arg(max_error_count);
            return;
        }
        maybeProcessEvents();
    }

    if (!wasTrac)
        warningNotHaveBlock(formatTrac);
    if (!wasMone)
        warningNotHaveBlock(formatMone);
    if (!wasShip)
        warningNotHaveBlock(formatShip);
    if (!wasIcee)
        infoNotHaveBlock(formatIcee);

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

        if (in.lineIsBlockEnd() || checkForOtherBlockStart()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 5) {
            warningArgCount(5);
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

        if (in.lineIsBlockEnd() || checkForOtherBlockStart()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 3 && in.argSize() != 4) {
            warningArgCount({3, 4});
            continue;
        }

        Ship ship;

        if (in.argRefAt(0) == 'S')
            ship.type = Ship::Type::shooter;
        else if (in.argRefAt(0) == 'H')
            ship.type = Ship::Type::handler;
        else {
            warningBadShipType(in.argAt(0));
            continue;
        }
        ship.name = in.argAt(1);
        if (!convertWithWarnings(ship.speed, 2))
            continue;
        const bool hViaType{ ship.type == Ship::Type::handler };
        const bool hViaArgC{ in.argSize() == 4 };
        if ((hViaType || hViaArgC) != (hViaType && hViaArgC)) {
            qWarning().noquote() << QString("Строка %1. Ошибка при чтении в %2: корабль имеет несоответствие данных: "
                                            "%3 с %4 входными аргументами. Данная строка не будет учтена. "
                                            "Исходный текст строки: '%5'")
                                    .arg(in.strLineNumber()).arg(in.currentFormatString).arg(hViaType ? "укладчик" : "шутер")
                                    .arg(in.argSize()).arg(in.currentLine);
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

        if (in.lineIsBlockEnd() || checkForOtherBlockStart()) {
            return;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 1) {
            warningArgCount(1);
            continue;
        }

        double money;
        if (!convertWithWarnings(money, 0))
            return;
        data->sensorMone.setMoney(money);
        break;
    }

    while (true) {
        if (in.fileAtEnd()) {
            warningUnexpectedEndOfFile();
            break;
        }

        in.readLine();

        if (in.lineIsBlockEnd() || checkForOtherBlockStart()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 3) {
            warningArgCount(3);
            continue;
        }

        ShipMone shipMone;

        if (in.argRefAt(0) == 'S')
            shipMone.type = Ship::Type::shooter;
        else if (in.argRefAt(0) == 'H')
            shipMone.type = Ship::Type::handler;
        else {
            warningBadShipType(in.argAt(0));
            continue;
        }

        shipMone.name = in.argAt(1);

        if (!convertWithWarnings(shipMone.money, 2))
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

        if (in.lineIsBlockEnd() || checkForOtherBlockStart()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (in.argSize() != 3) {
            warningArgCount(3);
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

bool SourceFileReader::pathLikelyShip() const
{
    return in.argSize() == 3 && (in.argRefAt(0) == 'H' || in.argRefAt(0) == 'S');
}

bool SourceFileReader::pathLikelyTrac() const
{
    auto isInt = [](const QStringRef &string) {
        bool ok;
        string.toInt(&ok);
        return ok;
    };

    if (in.argSize() != 4)
        return false;

    for (int i = 0; i < in.argSize(); ++i) {
        if (!isInt(in.argRefAt(i)))
            return false;
    }
    return true;
}


void SourceFileReader::readPath()
{
    while (true) {
        if (in.fileAtEnd()) {
            warningUnexpectedEndOfFile();
            break;
        }

        in.readLine();

        if (in.lineIsBlockEnd() || checkForOtherBlockStart()) {
            break;
        }

        if (in.lineIsEmpty()) {
            warningEmptyString();
            continue;
        }

        if (pathLikelyTrac()) {
            qWarning().noquote() << QString("Строка %1. Выглядит как запись пути корабля, но нет предшествующей записи "
                                            "о принадлежности к судну")
                                    .arg(in.strLineNumber());
            continue;
        }

        if (in.argSize() != 3) {
            warningArgCount(3);
            continue;
        }

        Path shipPath;
        if (in.argRefAt(0) == 'S')
            shipPath.type = Ship::Type::shooter;
        else if (in.argRefAt(0) == 'H')
            shipPath.type = Ship::Type::handler;
        else {
            warningBadShipType(in.argAt(0));
            continue;
        }
        shipPath.name = in.argAt(1);
        if (!convertWithWarnings(shipPath.size, 2))
            continue;

        if (shipPath.size < 1) {
            qWarning().noquote() << QString("Строка %1. Ошибка при чтении строки в %2: Число записей маршрута корабля %3 должно быть "
                                            "положительным, но встречено ( %4 )")
                                    .arg(in.strLineNumber()).arg(in.currentFormatString).arg(shipPath.name).arg(shipPath.size);
        }

        shipPath.path.reserve(shipPath.size);

        // чтение точек для пути корабля
        int i = 0;
        while (true) {
            if (in.fileAtEnd()) {
                qWarning().noquote() << QString("Строка %1. Ошибка при чтении точки в %2: Встречен конец файла, хотя ожидалось "
                                                "получить ещё %3 строк для корабля %4")
                                        .arg(in.strLineNumber()).arg(in.currentFormatString).arg(shipPath.size - i)
                                        .arg(shipPath.name);
                return;
            }
            in.readLine();

            if (in.lineIsBlockEnd()) {
                if (shipPath.size - i > 0) {
                    qWarning().noquote() << QString("Строка %1. Ошибка при чтении точки в %2: Встречен конец блока, хотя ожидалось "
                                                    "получить ещё %3 строк для корабля %4")
                                            .arg(in.strLineNumber()).arg(in.currentFormatString).arg(shipPath.size - i)
                                            .arg(shipPath.name);
                }
                in.reverseReadLine();
                break;
            }
            if (checkForOtherBlockStart()) {
                if (shipPath.size - i > 0) {
                    qWarning().noquote() << QString("Строка ~%1. Ошибка при чтении точки в %2: Встречено начало нового блока, хотя ожидалось "
                                                    "получить ещё %3 строк для корабля %4")
                                            .arg(in.strLineNumber()).arg(in.currentFormatString).arg(shipPath.size - i)
                                            .arg(shipPath.name);
                }
                break;
            }

            if (in.lineIsEmpty()) {
                // мы не должны учитывать пустые строки
                warningEmptyString();
                continue;
            }

            if (pathLikelyShip()) {
                if (shipPath.size - i > 0) {
                    qWarning().noquote() << QString("Строка %1. Ошибка при чтении строки в %2: Встречена запись, похожая "
                                                    "на начало маршрута следующего корабля, но для текущего маршрута "
                                                    "судна %4 ожидалось получить ещё %3 строк. Действие по умолчанию - "
                                                    "начать считывание следующего маршрута. Входная строка ( %5 )")
                                            .arg(in.strLineNumber()).arg(in.currentFormatString).arg(shipPath.size - i)
                                            .arg(shipPath.name).arg(in.currentLine);
                }
                in.reverseReadLine();
                break;
            }

            if (in.argSize() != 4) {
                warningArgCount(4);

                ++i;
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

            ++i;

            // мы вышли за границы массива, но вдруг там есть ещё точки?
            if (i >= shipPath.size) {
                // проверим следующую строку на корректность
                in.readLine();
                if (!pathLikelyTrac()) {
                    in.reverseReadLine();
                    // следующая строка не является продолжением
                    if (i != shipPath.size) {
                        qWarning().noquote() << QString("Строка ~%1. В %2 для маршрута корабля %3 ожидалось "
                                                        "встретить %4 точек, но по содержимому файла было получено %5 "
                                                        "(больше)")
                                                .arg(in.strLineNumber()).arg(in.currentFormatString).arg(shipPath.name)
                                                .arg(shipPath.size).arg(i-1);
                    }
                    break;
                }
                in.reverseReadLine();
            }
        }

        data->path.append(shipPath);
    }
}

void SourceFileReader::readFictive()
{
    while (true) {
        if (in.fileAtEnd())
            break;

        in.readLine();

        if (in.lineIsBlockEnd())
            break;
    }
}

bool SourceFileReader::checkForOtherBlockStart()
{
    if (in.testFormat(formatTrac)) {
        warningOtherBlockStart(formatTrac);
    }
    else if (in.testFormat(formatShip)) {
        warningOtherBlockStart(formatShip);
    }
    else if (in.testFormat(formatMone)) {
        warningOtherBlockStart(formatMone);
    }
    else if (in.testFormat(formatIcee)) {
        warningOtherBlockStart(formatIcee);
    }
    else if (in.testFormat(formatPath)) {
        warningOtherBlockStart(formatPath);
    }
    else {
        return false;
    }

    in.reverseReadLine();
    return true;
}

void SourceFileReader::warningArgCount(const QVector<int> &expected)
{
    QStringList expArgs;
    for (int i = 0; i < expected.size(); ++i) {
        expArgs << QString::number(expected.at(i));
    }
    const QString expString{ expArgs.join(" или ") };
    qWarning().noquote() << QString("Строка %1. Ошибка при чтении в %2: строка имеет некорректное ожидаемому число "
                                    "аргументов (получено %3, ожидалось %4). Данная строка не будет учтена. Исходный "
                                    "текст строки: '%5'")
                            .arg(in.strLineNumber()).arg(in.currentFormatString).arg(in.argSize()).arg(expString).arg(in.currentLine);
}


void SourceFileReader::warningArgConvertToInt(int badArgNumber)
{
    qWarning().noquote() << QString("Строка %1. Ошибка при чтении в %2: строка имеет неконвертируемую в int%6 запись. "
                                    "Данная строка не будет учтена. Аргумент по номеру %3: '%4'. Исходный "
                                    "текст строки: '%5'")
                            .arg(in.strLineNumber()).arg(in.currentFormatString).arg(badArgNumber)
                            .arg(in.argRefAt(badArgNumber)).arg(in.currentLine).arg(sizeof (int) * 8);
}

void SourceFileReader::warningBadShipType(const QString &type)
{
    qWarning().noquote() << QString("Строка %1. Ошибка при чтении в %2: корабль имеет некорректный тип '%3'. "
                                    "Данная строка не будет учтена. Исходный текст строки: '%4'")
                            .arg(in.strLineNumber()).arg(in.currentFormatString).arg(type).arg(in.currentLine);
}

void SourceFileReader::warningUnexpectedEndOfFile()
{
    qWarning().noquote() << QString("Строка в конце файла (примерно %1). Ошибка при чтении в %2: обнаружен конец файла, но текущий блок не был закрыт.")
                            .arg(in.lineNumber()).arg(in.currentFormatString);
}

void SourceFileReader::warningEmptyString()
{
    // не будем писать что-то такое...
    //qWarning().noquote()  << QString("Строка %1. Предупреждение. Встречена пустая строка в файле исходных данных")
    //                      .arg(in.strLineNumber());
}

void SourceFileReader::warningExtraBlock(const char *blockName)
{
    qWarning().noquote()  << QString("Строка %1. Встречено второе объявление блока %2. Данные будут полностью проигнорированы")
                             .arg(in.strLineNumber()).arg(blockName);
}

void SourceFileReader::infoNotHaveBlock(const char *blockName)
{
    qInfo().noquote()  << QString("Предупреждение. В файле отсутствует блок данных %1")
                          .arg(blockName);
}

void SourceFileReader::warningNotHaveBlock(const char *blockName)
{
    qWarning().noquote()  << QString("Ошибка: в файле отсутствует обязательный блок данных %1")
                             .arg(blockName);
}

void SourceFileReader::warningOtherBlockStart(const char *otherBlockName)
{
    qWarning().noquote()  << QString("Строка %1. При чтении блока %2 встречено начало следующего блока %3 - "
                                     "вероятно пропущен символ разделения блоков. Следующие строки будут "
                                     "учитываться как данные блока %3")
                             .arg(in.strLineNumber()).arg(in.currentFormatString).arg(otherBlockName);
}
