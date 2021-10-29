#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QMimeData;
class SimulationScene;
class QAction;

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

private:
    Ui::MainWindow *ui;
    SimulationScene *scene{ nullptr };
    QAction *pastAction{ nullptr };
};
#endif // MAINWINDOW_H
