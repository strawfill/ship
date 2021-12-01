#include "waitingframe.h"

#include "algodummy.h"
#include "ignoredndgraphicsview.h"
#include "simulationscene.h"
#include "prepareddata.h"

#include <QVBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QResizeEvent>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>

#include <QtMath>

#if 1
#include <QDebug>
#include <QElapsedTimer>
#endif

WaitingFrame::WaitingFrame(QWidget *parent)
    : QFrame(parent)
    , viewer(new IgnoreDndGraphicsView(this))
    , scene(new SimulationScene(this))
    , label(new QLabel(this))
    , progressBar(new QProgressBar(this))
{
    setFrameStyle(QFrame::NoFrame);

    auto hlayout{ new QVBoxLayout(this) };
    label->setAlignment(Qt::AlignCenter);
    hlayout->addWidget(viewer);
    hlayout->setMargin(0);
    //hlayout->addWidget(label);
    //hlayout->setStretch(0, 1);

    viewer->setRenderHint(QPainter::Antialiasing);
    viewer->setRenderHint(QPainter::TextAntialiasing);
    viewer->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewer->setMinimumSize(250, 250);
    viewer->setScene(scene->getScene());

    progressBar->setValue(0);
    progressBar->setFixedHeight(4);
    progressBar->setTextVisible(false);

    setRuleForce(Rule::see);

    scene->setSimulationSpeed(10);

    label->setText("Подождите, идёт создание маршрута...");

    connect(scene, &SimulationScene::startPauseChanged, this, &WaitingFrame::simulationStateChanged);
}

void WaitingFrame::setRule(WaitingFrame::Rule type)
{
    if (rule != type)
        setRuleForce(rule);
}

void WaitingFrame::setRuleForce(WaitingFrame::Rule type)
{
    label->setVisible(type == Rule::wait);
    progressBar->setVisible(type == Rule::wait);
}

void WaitingFrame::showEvent(QShowEvent *event)
{
    show = true;
    createSimulation();

    QFrame::showEvent(event);
}

void WaitingFrame::hideEvent(QHideEvent *event)
{
    show = false;
    scene->stopSimulation();
    scene->clear();

    QFrame::hideEvent(event);
}

void WaitingFrame::createSimulation()
{
    scene->clear();

    using namespace prepared;

    DataStatic ds;
    ds.handlers.append(Handler({}, 5 + qrand() % 5, 20000, 1, 1));
    ds.shooters.append(Shooter({}, 5 + qrand() % 5, 1));
    ds.applyAddedShips();

    auto rand = [](){ return 100 * (qrand() % 1001 - 500); };

    for (int i = 0; i < 3; ++i) {
        ds.tracs.append(Trac(raw::Trac{rand(), rand(), rand(), rand(), 1000}));
    }
    ds.applyAddedTracs();

    QElapsedTimer tm; tm.start();
    DataDynamic dd{ AlgoDummy(ds).find() };

    scene->setSources(ds, dd);
    scene->zoomSimulation(0.7);
    viewer->setSceneRect(QRectF{-100, -100, 200, 200});

    QTimer::singleShot(700, scene, &SimulationScene::startSimulation);
}

void WaitingFrame::simulationStateChanged()
{
    if (!show || scene->simulationActiveNow())
        return;


    QTimer::singleShot(700, this, &WaitingFrame::createSimulation);
}


void WaitingFrame::resizeEvent(QResizeEvent *event)
{
    QSize frameSize{ event->size() };

    label->resize(qRound(frameSize.width() / 1.5), label->height());
    QSize labelSize{ label->size() };
    label->move((frameSize.width()-labelSize.width())/2, frameSize.height()-labelSize.height() - 4);

    progressBar->resize(frameSize.width()-2, progressBar->height());
    progressBar->move(1, frameSize.height() - 5);


    QFrame::resizeEvent(event);
}

