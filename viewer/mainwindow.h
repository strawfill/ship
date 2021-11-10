#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QMimeData;
class SimulationScene;

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

private slots:
    void setStartPauseButtonPixmap(bool isStarted);

private:
    Ui::MainWindow *ui;
    SimulationScene *scene{ nullptr };

    QString fileData;
};
#endif // MAINWINDOW_H
