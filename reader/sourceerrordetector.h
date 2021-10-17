#ifndef SOURCEERRORDETECTOR_H
#define SOURCEERRORDETECTOR_H

namespace raw {
struct Data;
}

class SourceErrorDetector
{
public:
    SourceErrorDetector(const raw::Data &adata);
    ~SourceErrorDetector();

    void setDataAndDetectErrors(const raw::Data &adata);
    int errorCount() const { return errCount; }

    void clear();

private:
    void detectErrors();

    void errorsWithLimits();
    void errorsWithLimitsTrac();
    void errorsWithLimitsShip();
    void errorsWithLimitsMone();
    void errorsWithLimitsIcee();
    void errorsWithLimitsPath();

    void errorsWithShipNames();
    void errorsWithShipNamesUnic();
    void errorsWithShipNamesUnknown();

private:
    raw::Data *data{ nullptr };
    int errCount{ 0 };
};

#endif // SOURCEERRORDETECTOR_H
