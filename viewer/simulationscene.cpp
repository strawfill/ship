#include "simulationscene.h"

#include "prepareddata.h"
#include "tracgraphicsitem.h"
#include "shipgraphicsitem.h"

#include <QGraphicsScene>
#include <QKeySequence>
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

    clear();
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

    scene->setSceneRect(QRect());
    // добавим плейсхолдер
    scene->addText("Для начала работы перетащите или вставьте через " +
                   QKeySequence(QKeySequence::Paste).toString() +
                   "\n       сюда файл или текст с исходными данными");
}

void SimulationScene::startSimulation()
{
    if (!data)
        return;

    // если нажать на старт симуляции, когда она уже идёт, то время сбросится до ближайшей паузы
    // или смены скорости. Это баг, который я назвал фичей. К кнопке добавлено всплывающее описание,
    // не прикопаться

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

namespace {

using PathDotPair = QPair<PathDot, PathDot>;
using PathDotPairVector = QVector<PathDotPair>;
using IntPair = QPair<int, int>;
using IntPairVector = QVector<IntPair>;
using Line2PDPV = QMap<Line, PathDotPairVector>;

Line2PDPV getTracToPathMap(const DataStatic &ds, const DataDynamic &dd) {
    // заполним для каждой трассы действия кораблей
    Line2PDPV map;

    for (const auto & trac : ds.tracs)
        map.insert(trac.line(), {});

    auto fillFrom = [&map, &ds](const QVector<PathDot> &pd) {
        for (int i = 1; i < pd.size(); ++i) {
            const auto & bef{ pd.at(i-1) };
            const auto & cur{ pd.at(i) };

            if (bef.activity == sa_layout || bef.activity == sa_collection || bef.activity == sa_shooting) {

                const Trac trac{ ds.tracViaLine(Line{ bef.x, bef.y, cur.x, cur.y }) };

                // это проверяется в другом месте, здесь же будет только мешать. Пропустим
                if (!trac.valid())
                    continue;

                // тут нет лишних действий, trac.line() не обязательно равен Line{ bef.x, bef.y, cur.x, cur.y }

                auto vector{ map.value(trac.line()) };
                vector.append({bef, cur});
                map.insert(trac.line(), vector);
            }
        }
    };

    fillFrom(dd.pathHandler);
    fillFrom(dd.pathShooter);

    return map;
}

IntPairVector getActionTimesForLine(const Line &line, const Line2PDPV &map)
{
    const auto & v{ map.value(line) };

    if (v.size() != 3)
        return {};

    IntPair layout, shooting, collection;

    for (int i = 0; i < v.size(); ++i) {
        const auto pair{ v.at(i) };
        if (pair.first.activity == sa_layout) {
            layout = { pair.first.timeH, pair.second.timeH };
        }
        else if (pair.first.activity == sa_shooting) {
            shooting = { pair.first.timeH, pair.second.timeH };
        }
        else if (pair.first.activity == sa_collection) {
            collection = { pair.first.timeH, pair.second.timeH };
        }
    }

    IntPairVector result;
    result.reserve(3);
    result.append(layout);
    result.append(shooting);
    result.append(collection);

    return result;
}
} // end anonymous namespace

void SimulationScene::initSceneItems()
{
    if (!data)
        return;

    // уберём плейсхолдер
    scene->clear();

    // чтобы начало координат было
    scene->addLine(-100, 0, 100, 0, QPen{Qt::gray});
    scene->addLine(0, -100, 0, 100, QPen{Qt::gray});

    auto map{ getTracToPathMap(data->ds, data->dd) };

    data->tracs.reserve(data->ds.tracs.size());
    for (const auto & t : qAsConst(data->ds.tracs)) {
        auto item{ new TracGraphicsItem(t, getActionTimesForLine(t.line(), map)) };
        scene->addItem(item);
        data->tracs.append(item);
    }

    data->handler = new ShipGraphicsItem(raw::Ship::Type::handler,
                                         data->ds.handlerViaName(data->dd.handlerName).speed(),
                                         data->dd.pathHandler);
    scene->addItem(data->handler);

    data->shooter = new ShipGraphicsItem(raw::Ship::Type::shooter,
                                         data->ds.shooterViaName(data->dd.shooterName).speed(),
                                         data->dd.pathShooter);
    scene->addItem(data->shooter);

    QRectF rect{-200, -200, 400, 400};
    rect |= scene->itemsBoundingRect().translated(100, 100);
    rect |= scene->itemsBoundingRect().translated(-100, -100);
    scene->setSceneRect(rect);
}

void SimulationScene::updateScene()
{
    if (!data)
        return;

    for (const auto & p : qAsConst(data->tracs))
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
    // сделаем красивую строчку, длина которой не будет постоянно прыгать от числа к числу
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
