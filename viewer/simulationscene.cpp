#include "simulationscene.h"

#include "prepareddata.h"
#include "tracgraphicsitem.h"
#include "shipgraphicsitem.h"

#include <QGraphicsScene>
#include <QTimer>

using namespace prepared;


struct SimulationData
{
    DataStatic  ds;
    DataDynamic dd;

    QVector<TracGraphicsItem *> tracs;
    ShipGraphicsItem *handler;
    ShipGraphicsItem *shooter;
};





SimulationScene::SimulationScene(QObject *parent)
    : QObject(parent)
    , scene(new QGraphicsScene(this))
    , tickTimer(new QTimer)
{
    tickTimer->setInterval(10); // 100 Гц, мой новый ноутбук крутой и такое отобразит и посчитает
    connect(tickTimer, &QTimer::timeout, this, &SimulationScene::timerTicked);
}

SimulationScene::~SimulationScene()
{
    clear();
    delete tickTimer;
}

void SimulationScene::setSources(const prepared::DataStatic &staticData, const prepared::DataDynamic &dynamicData)
{
    clear();
    data = new SimulationData;
    data->ds = staticData;
    data->dd = dynamicData;
    calculateEndSimulationTime();
    initSceneItems();

    // и сразу зададим нулевой час, чтобы элементы корректно отображались
    updateScene();
}

void SimulationScene::clear()
{
    // очистка сцены удалит все прикреплённые к ней элементы
    scene->clear();
    delete data;
    data = nullptr;

    tickTimer->stop();
    resetTime();
}

void SimulationScene::startSimulation()
{
    if (!data)
        return;

    // мы обнулили это время, но оставили hourBase
    hour = 0;

    eltimer.restart();

    tickTimer->start();
}

void SimulationScene::pauseSimulation()
{
    // запомним текущее состояние
    hourBase = hour;

    tickTimer->stop();
}

void SimulationScene::stopSimulation()
{
    pauseSimulation();
    resetTime();
    updateScene();

    emitSimulationTimeChanged();
}

void SimulationScene::setSimulationSpeed(double hoursInSec)
{
    // мы считаем скорость по реальному времени
    // (иначе, если складывать timeout от QTimer, то выйдет меньше)
    // но если пользователь изменит скорость, прямая формула расчёта перестанет работать, поэтому
    // введён параметр hourBase, который запоминает то, что было
    hourBase = hour;
    eltimer.restart();
    speed = hoursInSec;
}

void SimulationScene::timerTicked()
{
    hour = hourBase + eltimer.elapsed() * 0.001 * speed;

    // достигли конца, поэтому можно и остановиться
    if (!runAfterEnd && endSimulationTime > 0 && hour > endSimulationTime) {
        hour = endSimulationTime; // чтобы ровное число было, хотя на самом деле так писать не очень правильно
        runAfterEnd = true;
        pauseSimulation();
    }

    updateScene();

    emitSimulationTimeChanged();

}

void SimulationScene::initSceneItems()
{
    if (!data)
        return;

    scene->addLine(-100, 0, 100, 0, QPen{Qt::gray});
    scene->addLine(0, -100, 0, 100, QPen{Qt::gray});

    data->tracs.reserve(data->ds.tracs.size());
    for (const auto & t : data->ds.tracs) {
        auto item{ new TracGraphicsItem(t) };
        scene->addItem(item);
        data->tracs.append(item);
    }

    data->handler = new ShipGraphicsItem(QPixmap{":/handler.png"},
                                         data->ds.handlerViaName(data->dd.handlerName).speed(),
                                         data->dd.pathHandler);
    scene->addItem(data->handler);

    data->shooter = new ShipGraphicsItem(QPixmap{":/shooter.png"},
                                         data->ds.shooterViaName(data->dd.shooterName).speed(),
                                         data->dd.pathShooter);
    scene->addItem(data->shooter);

    QRectF rect{-200, -200, 400, 400};
    rect |= scene->itemsBoundingRect().translated(100, 100);
    rect |= scene->itemsBoundingRect().translated(-100, 100);
    scene->setSceneRect(rect);
}

void SimulationScene::updateScene()
{
    if (!data)
        return;

    for (const auto & p : data->tracs)
        p->setHour(hour);

    data->handler->setHour(hour);
    data->shooter->setHour(hour);
}

void SimulationScene::resetTime()
{
    hour = 0;
    hourBase = 0;
    eltimer.restart();
    // это значит, что конец ещё не настал и при его преодолении стоит остановиться
    runAfterEnd = false;
}

void SimulationScene::calculateEndSimulationTime()
{
    endSimulationTime = 0;
    if (!data || !data->dd.has)
        return;

    if (!data->dd.pathHandler.isEmpty()) {
        endSimulationTime = data->dd.pathHandler.last().timeH;
    }

    if (!data->dd.pathShooter.isEmpty() && data->dd.pathShooter.last().timeH > endSimulationTime) {
        endSimulationTime = data->dd.pathShooter.last().timeH;
    }
}

void SimulationScene::emitSimulationTimeChanged()
{
    QString time;

    if (qFuzzyIsNull(hour))
        time = "0";
    else if (hour < 10.)
        time = QString::number(hour, 'f', 2);
    else if (hour < 100.)
        time = QString::number(hour, 'f', 1);
    else
        time = QString::number(hour, 'f', 0);

    emit simulationTimeChanged(time);
}
