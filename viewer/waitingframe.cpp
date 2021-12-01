#include "waitingframe.h"

#include "shipgraphicsitem.h"

#include <QVBoxLayout>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
#include <QTimerEvent>
#include <QLabel>

#include <QtMath>

WaitingFrame::WaitingFrame(QWidget *parent)
    : QFrame(parent)
{
    setFrameStyle(QFrame::NoFrame);

    auto hlayout{ new QVBoxLayout(this) };
    auto viewer{ new QGraphicsView(this) };
    auto label{ new QLabel(this) };
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

    label->setText("Подождите, идёт создание маршрута...");

    auto scene{ new QGraphicsScene(this) };


    viewer->setScene(scene);

    for (int i = 0; i < count; ++i) {
        auto h{ new ShipGraphicsItem(raw::Ship::Type::handler, 10, {}, 1) };
        auto s{ new ShipGraphicsItem(raw::Ship::Type::shooter, 10, {}, 1) };

        handlers.append(h);
        shooters.append(s);
        degrees.append(0);

        scene->addItem(h);
        scene->addItem(s);
    }
}


void WaitingFrame::showEvent(QShowEvent *event)
{
    setInitPos();

    QFrame::showEvent(event);

    timerId = startTimer(15);
}

void WaitingFrame::hideEvent(QHideEvent *event)
{
    if (timerId >= 0)
        killTimer(timerId);

    QFrame::hideEvent(event);
}

void WaitingFrame::timerEvent(QTimerEvent *event)
{
    QFrame::timerEvent(event);

    if (event->timerId() != timerId)
        return;

    setNextPos();
}

void WaitingFrame::setRotation()
{
    for (int i = 0; i < count; ++i) {
        setPlaceForShip(30 + 15*i, qRound(degrees.at(i)), handlers.at(i));
        setPlaceForShip(30 + 15*i, qRound(degrees.at(i)) + 180, shooters.at(i));
    }
}

void WaitingFrame::setPlaceForShip(double radius, int degree, QGraphicsPathItem *ship)
{
    const double y = qSin(degree * M_PI / 180) * radius;
    const double x = qCos(degree * M_PI / 180) * radius;

    ship->setPos(x, y);
    ship->setRotation(degree+180);
    ship->update();
}

void WaitingFrame::setInitPos()
{
    for (int i = 0; i < count; ++i)
        degrees[i] = 0;
}

void WaitingFrame::setNextPos()
{
    for (int i = 0; i < count; ++i) {
        degrees[i] += 2. / (1 + i);
    }

    setRotation();
}
