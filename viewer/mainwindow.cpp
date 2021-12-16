#include <QClipboard>
#include <QDebug>
#include <QFileInfo>
#include <QMimeData>
#include <QSettings>
#include <QTemporaryFile>
#include <QThread>

#include "algoannealing.h"
#include "algobruteforce.h"
#include "debugcatcher.h"
#include "graphicsitemzoomer.h"
#include "graphicsviewzoomer.h"
#include "patherrordetector.h"
#include "placeholderframe.h"
#include "prepareddata.h"
#include "rawdata.h"
#include "simulationscene.h"
#include "sourceerrordetector.h"
#include "sourcefilereader.h"
#include "movestopathconverter.h"
#include "waitingframe.h"
#include "worker.h"

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new SimulationScene(this))
    , placeholderFrame(new PlaceholderFrame(this))
    , waitingFrame(new WaitingFrame(this))
    , workerThread(new QThread(this))
    , worker(new Worker)
{
    ui->setupUi(this);
    // чтобы второй был минимального размера
    ui->splitter->setSizes({10000, 1});

    ui->simulationViewLayout->addWidget(placeholderFrame);
    ui->simulationViewLayout->addWidget(waitingFrame);
    setCurrentSimulationWindowForce(SimulationWindow::placeholder);

    connect(DebugCatcher::instance(), &DebugCatcher::messageRecieved, ui->plainTextEdit, &QPlainTextEdit::appendPlainText);

    connect(ui->tb_startpause, &QToolButton::clicked, scene, &SimulationScene::startPauseSimulation);
    connect(scene, &SimulationScene::startPauseChanged, this, &MainWindow::setStartPauseButtonPixmapState);
    //connect(ui->tb_pause, &QToolButton::clicked, scene, &SimulationScene::pauseSimulation);
    connect(ui->tb_stop, &QToolButton::clicked, scene, &SimulationScene::stopSimulation);
    connect(ui->doubleSpinBox_speed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), scene, &SimulationScene::setSimulationSpeed);
    scene->setSimulationSpeed(ui->doubleSpinBox_speed->value());

    connect(scene, &SimulationScene::simulationTimeChanged, ui->label_time, &QLabel::setText);

    ui->graphicsView->setScene(scene->getScene());
    viewZoomer = new GraphicsViewZoomer(ui->graphicsView);
    viewZoomer->set_enable(false);
    itemZoomer = new GraphicsItemZoomer(ui->graphicsView);
    connect(itemZoomer, &GraphicsItemZoomer::zoomRequested, scene, &SimulationScene::zoomSimulation);

    loadSettings();
    setStartPauseButtonPixmapState(false);

    initActions();
    prepareWorker();

    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->graphicsView->setOptimizationFlag(QGraphicsView::DontSavePainterState);
    // чтобы не было артефактов при перемещении сцены с помощью мыши
    // также это ускоряет перерисовку при большом количестве трасс
    ui->graphicsView->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    viewZoomer->set_enable(true);

#if 0
    QMimeData md;
    md.setUrls(QList<QUrl>() << QUrl{"file:///" + QDir::currentPath() + "/../input/simple.m/8.txt"});
    //md.setUrls(QList<QUrl>() << QUrl{"file:///" + QDir::currentPath() + "/../input/simple/_s10.txt"});
    //md.setUrls(QList<QUrl>() << QUrl{"file:///" + QDir::currentPath() + "/../input/test_brute_force/simple/test.txt"});
    processMimeData(&md);
#endif
}

void MainWindow::initActions()
{
    // добавим горячие клавиши для удобства

    auto paste{ new QAction(this) };
    addAction(paste);
    paste->setShortcut(QKeySequence::Paste);
    connect(paste, &QAction::triggered, this, &MainWindow::postFromClipboardRequested);

    const QVector<QKeySequence> ks {
        QKeySequence{Qt::Key_R},
        QKeySequence{Qt::Key_Space},
        QKeySequence{Qt::CTRL + Qt::Key_Space}
    };
    for (const auto & key : ks) {
        auto startPause{ new QAction(this) };
        startPause->setShortcut(key);
        connect(startPause, &QAction::triggered, scene, &SimulationScene::startPauseSimulation);
        addAction(startPause);
    }

    auto stop{ new QAction(this) };
    stop->setShortcut(QKeySequence{Qt::CTRL + Qt::Key_R});
    connect(stop, &QAction::triggered, scene, &SimulationScene::stopSimulation);
    addAction(stop);

    // включение режима просмотра
    auto seeing{ new QAction(this) };
    seeing->setShortcut(QKeySequence{Qt::CTRL + Qt::Key_T});
    connect(seeing, &QAction::triggered, this, &MainWindow::changePlaceholderSee);
    addAction(seeing);
}

void MainWindow::prepareWorker()
{
    worker->setProgressWatcher(waitingFrame->progressBarSetter());

    worker->moveToThread(workerThread);
    // теперь они в разных потоках и поэтому будет соединение через очередь
    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &MainWindow::startWorkerRequest, worker, &Worker::start);
    connect(this, &MainWindow::stopWorkerRequest, worker, &Worker::stop);
    connect(worker, &Worker::ended, this, &MainWindow::workerEndWork);
    workerThread->start();
}

void MainWindow::setCurrentSimulationWindow(MainWindow::SimulationWindow type)
{
    if (windowType != type)
        setCurrentSimulationWindowForce(type);
}

void MainWindow::setCurrentSimulationWindowForce(MainWindow::SimulationWindow type)
{
    windowType = type;
    ui->graphicsView->setVisible(type == SimulationWindow::viewer);
    ui->simulationParams->setVisible(type == SimulationWindow::viewer);
    placeholderFrame->setVisible(type == SimulationWindow::placeholder);
    waitingFrame->setVisible(type == SimulationWindow::waiter || type == SimulationWindow::see);

    if (type == SimulationWindow::waiter)
        waitingFrame->setRule(WaitingFrame::Rule::wait);
    else if (type == SimulationWindow::see)
        waitingFrame->setRule(WaitingFrame::Rule::see);
}

void MainWindow::changePlaceholderSee()
{
    if (windowType == SimulationWindow::placeholder)
        setCurrentSimulationWindow(SimulationWindow::see);
    else if (windowType == SimulationWindow::see)
        setCurrentSimulationWindow(SimulationWindow::placeholder);
}

MainWindow::~MainWindow()
{
    workerThread->quit();
    workerThread->wait();

    saveSettings();
    delete ui;
}

void MainWindow::loadSettings()
{
    // не хочу пачкать реестр
    QSettings s("settings.ini", QSettings::IniFormat);

    s.beginGroup("mainwindow");
    ui->splitter->restoreState(s.value("splitter").toByteArray());
    restoreGeometry(s.value("geometry").toByteArray());
    s.endGroup();


    s.beginGroup("simulation");
    ui->doubleSpinBox_speed->setValue(s.value("speed", 10.).toDouble());
    // spinBox не даст поставить что-то плохое
    scene->setSimulationSpeed(ui->doubleSpinBox_speed->value());
    s.endGroup();
}

void MainWindow::saveSettings()
{
    // не хочу пачкать реестр
    QSettings s("settings.ini", QSettings::IniFormat);

    s.beginGroup("mainwindow");
    s.setValue("splitter", ui->splitter->saveState());
    s.setValue("geometry", saveGeometry());
    s.endGroup();

    s.beginGroup("simulation");
    s.setValue("speed", ui->doubleSpinBox_speed->value());
    s.endGroup();
}

void MainWindow::setStartPauseButtonPixmapState(bool isStarted)
{
    if (isStarted) {
        ui->tb_startpause->setIcon(QIcon(":/pause.png"));
        ui->tb_startpause->setToolTip("Поставить на паузу\nR или [Ctrl+]Space");
    }
    else {
        ui->tb_startpause->setIcon(QIcon(":/start.png"));
        ui->tb_startpause->setToolTip("Запустить симуляцию\nR или [Ctrl+]Space");
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->setAccepted(hasGoodFormat(event->mimeData()));
}

void MainWindow::dropEvent(QDropEvent *event)
{
    processMimeData(event->mimeData());
}

void MainWindow::postFromClipboardRequested()
{
    processMimeData(QGuiApplication::clipboard()->mimeData());
}

bool MainWindow::hasGoodFormat(const QMimeData *data)
{
    if (!data)
        return false;

    // хочу просто текст кидать
    if (!data->hasUrls() && data->hasText())
        return true;

    if (!data->hasUrls() || data->urls().size() != 1)
        return false;
    const QString filename{ data->urls().constFirst().toLocalFile() };
    QFileInfo fileInfo(filename);
    if (!fileInfo.isFile())
        return false;
    if (!fileInfo.isReadable())
        return false;
    if (fileInfo.size() > 5 * 1024 * 1024)
        return false;

    return true;
}


void MainWindow::processMimeData(const QMimeData *data)
{
    if (!workerSleep)
        return;
    // пока что просто выйдем

    if (!data)
        return;

    if (!hasGoodFormat(data)) {
        setWindowTitle("StrawberryShip");
        return;
    }

    if (data->urls().size() == 1) {
        QString filename{ data->urls().constFirst().toLocalFile() };
        setWindowTitle("StrawberryShip @ " + filename);
        return processFile(filename);
    }

    if (data->hasText()) {
        setWindowTitle("StrawberryShip @ via text");
        QTemporaryFile tempfile;
        tempfile.open();
        tempfile.write(data->text().toUtf8());
        // вернёмся в начало, чтобы можно было что-то читать (хотя тут непонятно, как это работает...)
        tempfile.seek(0);
        return processFile(tempfile.fileName());
    }
}

namespace {

QString outputResult(const prepared::DataStatic & ds, const prepared::DataDynamic & dd)
{
    QString s{ QString::number(prepared::totalCost(ds, dd)) };
    int dots{ (s.size()-1) / 3 };
    int start{ s.size() % 3 };
    if (start == 0)
        start = 3;
    for (int i = 0; i < dots; ++i) {
        s.insert(start + i*4, '.');
    }

    return s + " [" + QString::number(prepared::totalHours(dd)) + " ч / " +
            QString::number(prepared::totalDays(dd)) + " дн. @ H: " +
            dd.handlerName + ", S: " + dd.shooterName + " ]";
};

}

void MainWindow::processFile(const QString &filename)
{
    Q_ASSERT(QFileInfo(filename).isFile());

    setCurrentSimulationWindow(SimulationWindow::placeholder);
    ui->graphicsView->resetTransform();

    DebugCatcher::instance()->clearWaringsCount();
    ui->plainTextEdit->clear();

    SourceFileReader reader(filename);
    SourceErrorDetector rawDetector(reader.dat());

    if (DebugCatcher::instance()->warningsCount())
        return;

    prepared::DataStatic ds(reader.dat());
    prepared::DataDynamic dd(reader.dat());

    // в ds конструкторе тоже несколько ошибок проверяется...
    if (DebugCatcher::instance()->warningsCount())
        return;

    if (dd.has) {
        PathErrorDetector pathErrorDetector(ds, dd);

        if (DebugCatcher::instance()->warningsCount())
            return;
    }
    // ошибок не обнаружено
    setCurrentSimulationWindow(SimulationWindow::waiter);



    ui->plainTextEdit->appendPlainText("-- ошибок не обнаружено");
    if (dd.has) {


        ui->plainTextEdit->appendPlainText(
                    "-- стоимость аренды по маршруту из исходных данных равна " + outputResult(ds, dd));

        // отрисуем пустые трассы
        scene->setSources(ds, dd);
        setCurrentSimulationWindow(SimulationWindow::viewer);
        return;
    }

    staticDataString.clear();
    QFile origin(filename);
    if (origin.open(QIODevice::Text|QIODevice::ReadOnly)) {
        origin.seek(0);
        staticDataString = origin.readAll();
        origin.close();
    }

    worker->setData(ds);
    emit startWorkerRequest();
}

void MainWindow::workerEndWork()
{
    workerSleep = true;

    auto ds{ worker->getInitData() };
    auto dd{ worker->getData() };

    scene->setSources(ds, dd);
    setCurrentSimulationWindow(SimulationWindow::viewer);

    if (dd.has) {
        ui->plainTextEdit->appendPlainText(
                    "-- стоимость аренды по созданному маршруту равна " + outputResult(ds, dd));

        QFile file("out.txt");
        if (file.open(QIODevice::Text|QIODevice::ReadWrite|QIODevice::Truncate)) {
            QTextStream ts(&file);
            ts << staticDataString;
            ts << "\n";
            ts << dd.toString();

            file.close();
        }

#if 0 // самотестирование, в целом потом стоит убрать
        processFile("out.txt");
#endif
    }
    else {
        ui->plainTextEdit->appendPlainText("-- маршрут не был создан, что-то пошло не так...");
    }
}
