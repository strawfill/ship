#ifndef WAITINGFRAME_H
#define WAITINGFRAME_H


#include <QFrame>
#include <QVector>

class QGraphicsPathItem;

class WaitingFrame : public QFrame
{
public:
    explicit WaitingFrame(QWidget *parent = nullptr);

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    void setRotation();
    void setPlaceForShip(double radius, int degree, QGraphicsPathItem *ship);

    void setInitPos();
    void setNextPos();

private:
    enum { count = 5 };
    QVector<QGraphicsPathItem *> handlers;
    QVector<QGraphicsPathItem *> shooters;
    QVector<double> degrees;
    int timerId{ -1 };
};

#endif // WAITINGFRAME_H
