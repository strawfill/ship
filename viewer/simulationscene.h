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

    bool simulationActiveNow() const;

    QGraphicsScene* getScene() const { return scene; }

signals:
    void simulationTimeChanged(const QString &hour);
    void startPauseChanged(bool isStartNow);

public slots:
    void startSimulation();
    void pauseSimulation();
    // в зависимости от текущего
    void startPauseSimulation();
    void stopSimulation();
    void setSimulationSpeed(double hoursInSec);

    void zoomSimulation(double factor);

private:
    void initSceneItems();
    void updateScene();
    void resetTime();

    void calculateInitZoomFactor();
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
    double distanceModifier{ 1 };

    int endSimulationTime{ 0 };
    bool runAfterEnd{ false };
};

#endif // SIMULATIONSCENE_H
