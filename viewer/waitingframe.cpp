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
    , labelTimer(new QTimer(this))
    , startSimulationDelay(new QTimer(this))
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
    progressBar->setFixedHeight(6);
    progressBar->setTextVisible(false);

    setRuleForce(Rule::see);

    scene->setSimulationSpeed(10);


    connect(scene, &SimulationScene::startPauseChanged, this, &WaitingFrame::simulationStateChanged);
    connect(labelTimer, &QTimer::timeout, this, &WaitingFrame::updateDotsInLabel);
    connect(startSimulationDelay, &QTimer::timeout, this, &WaitingFrame::startSimulation);
}

void WaitingFrame::setRule(WaitingFrame::Rule type)
{
    if (rule != type)
        setRuleForce(type);
}

void WaitingFrame::changeProgressBarValue(int percents)
{
    progressBar->setValue(percents);
}

void WaitingFrame::setRuleForce(WaitingFrame::Rule type)
{
    rule = type;
    progressBar->setVisible(type == Rule::wait);

    if (type == Rule::wait)
        label->setText("Подождите, идёт создание маршрута...");
    else
        label->setText("Заставка. Для выхода нажмите Ctrl+T");
}

void WaitingFrame::showEvent(QShowEvent *event)
{
    show = true;
    createSimulation();
    labelTimer->start(1000);

    QFrame::showEvent(event);
}

void WaitingFrame::hideEvent(QHideEvent *event)
{
    show = false;
    scene->stopSimulation();
    scene->clear();
    labelTimer->stop();

    QFrame::hideEvent(event);
}

void WaitingFrame::createSimulation()
{
    scene->clear();

    using namespace prepared;

    DataStatic ds;
    ds.handlers.append(Handler({}, 5 + qrand() % 8, 20000, 1, 1));
    ds.shooters.append(Shooter({}, 5 + qrand() % 8, 1));
    ds.applyAddedShips();

    auto rand = [](){ return 100 * (qrand() % 1001 - 500); };

    for (int i = 0; i < 3; ++i) {
        ds.tracs.append(Trac(raw::Trac{rand(), rand(), rand(), rand(), 1000}));
    }
    ds.applyAddedTracs();

    QElapsedTimer tm; tm.start();
    DataDynamic dd{ AlgoDummy(ds).find() };

    viewer->setSceneRect({});
    scene->setSources(ds, dd);
    scene->zoomSimulation(1.3);

    resizeSceneToViewSize();

    startSimulationDelay->start(700);
}

void WaitingFrame::startSimulation()
{
    startSimulationDelay->stop();
    scene->startSimulation();
}

void WaitingFrame::simulationStateChanged()
{
    if (!show || scene->simulationActiveNow())
        return;


    QTimer::singleShot(700, this, &WaitingFrame::createSimulation);
}

void WaitingFrame::resizeSceneToViewSize()
{
    QSizeF sizeG{ viewer->sceneRect().size() };
    sizeG.rheight() += 20;
    sizeG.rwidth() += 20;
    QSizeF sizeP{ viewer->size() };

    const double param1{ (qFuzzyIsNull(sizeG.width()) ? 1. : sizeP.width() / sizeG.width()) };
    const double param2{ (qFuzzyIsNull(sizeG.height()) ? 1. : sizeP.height() / sizeG.height()) };

    double factor{ qMin(param1, param2) };

    QTransform transform;
    transform.scale(factor, factor);
    viewer->setTransform(transform);
}

void WaitingFrame::updateDotsInLabel()
{
    if (label->text().endsWith(".  "))
        label->setText(label->text().mid(0, label->text().size()-3) + ".. ");
    else if (label->text().endsWith(".. "))
        label->setText(label->text().mid(0, label->text().size()-3) + "...");
    else if (label->text().endsWith("..."))
        label->setText(label->text().mid(0, label->text().size()-3) + ".  ");
}

void WaitingFrame::resizeEvent(QResizeEvent *event)
{
    QSize frameSize{ event->size() };

    label->resize(qRound(frameSize.width() / 1.01), label->height());
    QSize labelSize{ label->size() };
    label->move((frameSize.width()-labelSize.width())/2, frameSize.height()-labelSize.height() - 4);

    progressBar->resize(frameSize.width()-2, progressBar->height());
    progressBar->move(1, frameSize.height() - progressBar->height() - 1);

    resizeSceneToViewSize();


    QFrame::resizeEvent(event);
}

