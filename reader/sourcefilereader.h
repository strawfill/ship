#ifndef SOURCEFILEREADER_H
#define SOURCEFILEREADER_H

#include <QFile>
#include <QString>
#include <QRegularExpression>

namespace raw {
struct Data;
}

/**
 * @brief Класс считывателя данных с файла
 *
 * Считывает данные из исходного файла
 * При этом делая первичные проверки на корректность структуры данных
 */
class SourceFileReader
{
public:
    SourceFileReader(const QString &filename = QString{});
    ~SourceFileReader();
    void readSourceFile(QString filename);
    void clear();

    bool empty() const { return !data; }
    raw::Data dat() const;
    const raw::Data &constDat() const;

private:
    // далее функции считывают данные до разделителя '\'
    // не проводят умную корректность данных на ограничения и связи, просто смотрят, чтобы число входных аргументов и их тип были корректны
    void readTrac();
    void readShip();
    void readMone();
    void readIcee();
    void readPath();
    void readFictive();
    bool checkForOtherBlockStart();

    void warningArgCount(int expected) { return warningArgCount(QVector<int>() << expected); }
    void warningArgCount(const QVector<int> &expected);
    void warningArgConvertToInt(int badArgNumber);
    void warningBadShipType(const QString &type);
    void warningUnexpectedEndOfFile();
    void warningEmptyString();
    void warningExtraBlock(const char *blockName);
    void infoNotHaveBlock(const char *blockName);
    void warningNotHaveBlock(const char *blockName);
    void warningOtherBlockStart(const char *otherBlockName);

    // конвертирует число и указывает на ошибки
    template<typename T>
    bool convertWithWarnings(T &result, int argN)
    {
        bool ok;
        result = in.argRefAt(argN).toInt(&ok);
        if (!ok) {
            warningArgConvertToInt(argN);
        }

        return ok;
    }

    // проверка текущей линии секции PATH, на что она больше похожа
    bool pathLikelyShip() const;
    bool pathLikelyTrac() const;

private:
    raw::Data *data{ nullptr };

    // чтобы не пробрасывать эти поля во многие функции, они устанавливаются здесь
    struct {
        QFile sourceFile;
        QString currentLine;
    private:
        QVector<QStringRef> currentArgs;
        qint64 pos{ 0 };
        int currentLineNumber{ 0 };
    public:
        const char* currentFormatString{ "" };

        static QString removeCommentsAndTrim(const QString &source) {
            const int commentIndex{ source.indexOf("--") };
            if (commentIndex == -1)
                return source.trimmed();
            return source.mid(0, commentIndex).trimmed();
        }

        void readLine() {
            pos = sourceFile.pos();
            ++currentLineNumber;
            currentLine = removeCommentsAndTrim(sourceFile.readLine());
            currentArgs = currentLine.splitRef(QRegularExpression("\\s+"), QString::SkipEmptyParts);
        }
        void reverseReadLine() {
            if (sourceFile.pos() == pos)
                return;

            sourceFile.seek(pos);
            --currentLineNumber;
        }

        bool testFormat(const char *format) {
            return currentLine == format;
        }

        bool testAndSetFormat(const char *format) {
            if (testFormat(format)) {
                currentFormatString = format;
                return true;
            }
            return false;
        }

        inline bool fileAtEnd() const {return sourceFile.atEnd(); }
        inline bool lineIsBlockEnd() const { return currentLine == '/'; }
        inline bool lineIsEmpty() const { return currentLine.isEmpty(); }
        inline int argSize() const { return currentArgs.size(); }
        inline QString argAt(int i) const { return currentArgs.at(i).toString(); }
        inline QStringRef argRefAt(int i) const { return currentArgs.at(i); }
        inline int lineNumber() const { return currentLineNumber; }
        inline QString strLineNumber() const {
            auto s{ QString::number(lineNumber()) };
            return QString().fill(' ', 3 - s.size()) + s;
        }
    } in;
};

#endif // SOURCEFILEREADER_H
