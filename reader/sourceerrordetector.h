#ifndef SOURCEERRORDETECTOR_H
#define SOURCEERRORDETECTOR_H

namespace raw {
struct Data;
}

/**
 * @brief Определяет и выводит в Warning() ошибки, связанные с некорректными значениями данных
 *
 * Данный класс проверяет:
 *   корректность лимитов (различных ограничений)
 *   наличие обязательных данных
 *   непротиворечивость данных внутри блока и между блоками
 */
class SourceErrorDetector
{
public:
    SourceErrorDetector(const raw::Data &adata);
    ~SourceErrorDetector();

    void setDataAndDetectErrors(const raw::Data &adata);

    void clear();

private:
    void detectErrors();

    void errorsWithLimits();
    void errorsWithLimitsTrac();
    void errorsWithLimitsShip();
    void errorsWithLimitsMone();
    void errorsWithLimitsIcee();
    void errorsWithLimitsPath();

    void errorsNoRequiredData();
    void errorsNoShips();
    void errorsNoTracs();
    void errorsNoMone();

    void errorsWithShipNames();
    void errorsWithShipNamesUnic();
    void errorsWithShipNamesUnknown();

    void errorsWithIceeIntersects();
    void errorsWithTracIntersects();

private:
    raw::Data *data{ nullptr };
};

#endif // SOURCEERRORDETECTOR_H
