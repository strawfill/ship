#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QMimeData;

class SimulationScene;
class PlaceholderFrame;
class WaitingFrame;

class GraphicsItemZoomer;
class GraphicsViewZoomer;
class QThread;
class Worker;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    // QWidget interface
protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    bool hasGoodFormat(const QMimeData *data);
    void processMimeData(const QMimeData *data);
    void processFile(const QString &filename);
    void postFromClipboardRequested();

    void loadSettings();
    void saveSettings();

    void initActions();
    void prepareWorker();

    enum class SimulationWindow { viewer, placeholder, waiter, see };
    void setCurrentSimulationWindow(SimulationWindow type);
    void setCurrentSimulationWindowForce(SimulationWindow type);

    void changePlaceholderSee();

private slots:
    void setStartPauseButtonPixmapState(bool isStarted);
    void workerEndWork();

signals:
    void startWorkerRequest();
    void stopWorkerRequest();

private:
    Ui::MainWindow *ui;
    SimulationScene *scene{ nullptr };
    PlaceholderFrame *placeholderFrame{ nullptr };
    WaitingFrame *waitingFrame{ nullptr };
    GraphicsItemZoomer *itemZoomer{ nullptr };
    GraphicsViewZoomer *viewZoomer{ nullptr };
    QThread *workerThread{ nullptr };
    Worker *worker{ nullptr };
    QString staticDataString;

    SimulationWindow windowType{ SimulationWindow::placeholder };
    bool workerSleep{ true };

};
#endif // MAINWINDOW_H
