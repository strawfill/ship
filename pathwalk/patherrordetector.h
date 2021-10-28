#ifndef PATHERRORDETECTOR_H
#define PATHERRORDETECTOR_H

#include <QString>
#include <QVector>

namespace prepared {
class DataStatic;
class DataDynamic;
}
namespace raw {
class PathDot;
}

class PathErrorDetector
{
public:
    PathErrorDetector(const prepared::DataStatic &staticData, const prepared::DataDynamic &dynamicData);
    ~PathErrorDetector();

    void setDataAndDetectErrors(const prepared::DataStatic &staticData, const prepared::DataDynamic &dynamicData);

    void clear();

private:
    void detectErrors() const;

    void detectStartEndErrors() const;

    void detectSpeedAndTimeErrors() const;
    void detectSpeedAndTimeErrorsFor(const QVector<raw::PathDot> &pd, int speed) const;

    void detectMismatchTracErrors() const;
    void detectMismatchTracErrorsFor(const QVector<raw::PathDot> &pd) const;

    void detectProcessingTracErrors() const;

    void detectNoSensorsErrors() const;

private:
    prepared::DataStatic  *sd{ nullptr };
    prepared::DataDynamic *dd{ nullptr };
};

#endif // PATHERRORDETECTOR_H
