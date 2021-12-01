#ifndef WAITINGFRAME_H
#define WAITINGFRAME_H

#include <QFrame>
#include <QVector>
#include <QTimer>

class SimulationScene;
class QGraphicsView;
class QLabel;
class QProgressBar;

class WaitingFrame : public QFrame
{
public:
    explicit WaitingFrame(QWidget *parent = nullptr);

    enum class Rule { wait, see };

    void setRule(Rule type);

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setRuleForce(Rule type);
    void createSimulation();
    void simulationStateChanged();

private:
    QGraphicsView *viewer{ nullptr };
    SimulationScene *scene{ nullptr };
    QLabel *label{ nullptr };
    QProgressBar *progressBar{ nullptr };
    Rule rule{ Rule::see };
    bool show{ false };
};

#endif // WAITINGFRAME_H
