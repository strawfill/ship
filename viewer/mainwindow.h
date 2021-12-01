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

    enum class SimulationWindow { viewer, placeholder, waiter, see };
    void setCurrentSimulationWindow(SimulationWindow type);
    void setCurrentSimulationWindowForce(SimulationWindow type);

private slots:
    void setStartPauseButtonPixmapState(bool isStarted);

private:
    Ui::MainWindow *ui;
    SimulationScene *scene{ nullptr };
    PlaceholderFrame *placeholderFrame{ nullptr };
    WaitingFrame *waitingFrame{ nullptr };
    GraphicsItemZoomer *itemZoomer{ nullptr };
    GraphicsViewZoomer *viewZoomer{ nullptr };

    SimulationWindow windowType{ SimulationWindow::placeholder };

};
#endif // MAINWINDOW_H
