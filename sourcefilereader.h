#ifndef SOURCEFILEREADER_H
#define SOURCEFILEREADER_H

#include <QFile>
#include <QString>
#include <QRegularExpression>

namespace sourceData {
struct Data;
}

class SourceFileReader
{
public:
    SourceFileReader(const QString &filename = QString{});
    ~SourceFileReader();
    void readSourceFile(QString filename);
    void clear();

    void print() const;

private:
    // далее функции считывают данные до разделителя '\'
    // не проводят умную корректность данных на ограничения и связи, просто смотрят, чтобы число входных аргументов и их тип были корректны
    void readTrac();
    void readShip();
    void readMone();
    void readIcee();
    void readPath();

    void warningArgCount();
    void warningArgConvertToDouble(int badArgNumber);
    void warningUnexpectedEndOfFile();
    void warningEmptyString();
    // конвертирует число и указывает на ошибки
    template<typename T>
    bool convertWithWarnings(T &result, int argN)
    {
        bool ok;
        result = in.argRefAt(argN).toDouble(&ok);
        if (!ok) {
            warningArgConvertToDouble(argN);
        }

        return ok;
    }

private:
    sourceData::Data *data{ nullptr };

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

        bool testAndSetFormat(const char *format) {
            if (currentLine == format) {
                currentFormatString = format;
                return true;
            }
            return false;
        }

        inline bool fileAtEnd() const {return sourceFile.atEnd(); }
        inline bool lineIsBlockEnd() const { return currentLine == formatSeparator; }
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

    static constexpr const char* formatTrac{ "TRAC" };
    static constexpr const char* formatShip{ "SHIP" };
    static constexpr const char* formatMone{ "MONE" };
    static constexpr const char* formatIcee{ "ICEE" };
    static constexpr const char* formatPath{ "PATH" };
    static constexpr const char formatSeparator{ '/' };
};

#endif // SOURCEFILEREADER_H
