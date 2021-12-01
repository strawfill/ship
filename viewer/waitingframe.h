#ifndef WAITINGFRAME_H
#define WAITINGFRAME_H

#include <QFrame>
#include <QVector>
#include <QTimer>

class SimulationScene;
class QGraphicsView;
class QLabel;
class QTimer;
class QProgressBar;

class WaitingFrame : public QFrame
{
    Q_OBJECT
public:
    explicit WaitingFrame(QWidget *parent = nullptr);

    enum class Rule { wait, see };

    void setRule(Rule type);

public slots:
    void changeProgressBarValue(int percents);

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setRuleForce(Rule type);
    void createSimulation();
    void startSimulation();
    void simulationStateChanged();
    void resizeSceneToViewSize();
    void updateDotsInLabel();

private:
    QGraphicsView *viewer{ nullptr };
    SimulationScene *scene{ nullptr };
    QLabel *label{ nullptr };
    QProgressBar *progressBar{ nullptr };
    QTimer *labelTimer{ nullptr };
    QTimer *startSimulationDelay{ nullptr };
    Rule rule{ Rule::see };
    bool show{ false };
};

#endif // WAITINGFRAME_H
