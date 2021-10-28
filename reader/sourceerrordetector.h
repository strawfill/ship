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
    void detectErrors() const;

    void errorsWithLimits() const;
    void errorsWithLimitsTrac() const;
    void errorsWithLimitsShip() const;
    void errorsWithLimitsMone() const;
    void errorsWithLimitsIcee() const;
    void errorsWithLimitsPath() const;

    void errorsNoRequiredData() const;
    void errorsNoShips() const;
    void errorsNoTracs() const;
    void errorsNoMone() const;

    void errorsWithShipNames() const;
    void errorsWithShipNamesUnic() const;
    void errorsWithShipNamesUnknown() const;

    void errorsWithIceeIntersects() const;
    void errorsWithTracIntersects() const;

private:
    raw::Data *data{ nullptr };
};

#endif // SOURCEERRORDETECTOR_H
