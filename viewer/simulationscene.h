#ifndef SIMULATIONSCENE_H
#define SIMULATIONSCENE_H

#include <QObject>
#include <QElapsedTimer>

class QGraphicsScene;
class QTimer;

namespace prepared {
class DataStatic;
class DataDynamic;
}

struct SimulationData;

class SimulationScene : public QObject
{
    Q_OBJECT
public:
    explicit SimulationScene(QObject *parent = nullptr);
    ~SimulationScene();

    void setSources(const prepared::DataStatic &staticData, const prepared::DataDynamic &dynamicData);
    void clear();

    double simulationTime() const { return hour; }
    double simulationSpeed() const { return speed; }

    QGraphicsScene* getScene() const { return scene; }

signals:
    void simulationTimeChanged(const QString &hour);

public slots:
    void startSimulation();
    void pauseSimulation();
    void stopSimulation();
    void setSimulationSpeed(double hoursInSec);

private:
    void initSceneItems();
    void updateScene();
    void resetTime();

    void calculateEndSimulationTime();

    void emitSimulationTimeChanged();

private slots:
    void timerTicked();

private:
    QGraphicsScene *scene{ nullptr };
    QTimer *tickTimer{ nullptr };
    SimulationData *data{ nullptr };

    double hour{ 0 };
    double hourBase{ 0 };
    QElapsedTimer eltimer;
    double speed{ 1 };

    int endSimulationTime{ 0 };
    bool runAfterEnd{ false };
};

#endif // SIMULATIONSCENE_H