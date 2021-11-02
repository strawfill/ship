#include <QClipboard>
#include <QDebug>
#include <QFileInfo>
#include <QMimeData>
#include <QSettings>
#include <QTemporaryFile>
#include <QDir> // remove me

#include "algobruteforce.h"
#include "debugcatcher.h"
#include "graphicsviewzoomer.h"
#include "prepareddata.h"
#include "patherrordetector.h"
#include "rawdata.h"
#include "simulationscene.h"
#include "sourceerrordetector.h"
#include "sourcefilereader.h"
#include "movestopathconverter.h"

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new SimulationScene(this))
{
    ui->setupUi(this);
    // пока он не имеет смысла, и не понятно, будет ли иметь в будущем
    ui->groupBox_optimisation->hide();
    // чтобы второй был минимального размера
    ui->splitter->setSizes({10000, 1});

    connect(DebugCatcher::instance(), &DebugCatcher::messageRecieved, ui->plainTextEdit, &QPlainTextEdit::appendPlainText);

    connect(ui->tb_startpause, &QToolButton::clicked, scene, &SimulationScene::startPauseSimulation);
    connect(scene, &SimulationScene::startPauseChanged, this, &MainWindow::setStartPauseButtonPixmap);
    //connect(ui->tb_pause, &QToolButton::clicked, scene, &SimulationScene::pauseSimulation);
    connect(ui->tb_stop, &QToolButton::clicked, scene, &SimulationScene::stopSimulation);
    connect(ui->doubleSpinBox_speed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), scene, &SimulationScene::setSimulationSpeed);
    scene->setSimulationSpeed(ui->doubleSpinBox_speed->value());

    connect(scene, &SimulationScene::simulationTimeChanged, ui->label_time, &QLabel::setText);

    ui->graphicsView->setScene(scene->getScene());

    new GraphicsViewZoomer(ui->graphicsView);

    loadSettings();
    setStartPauseButtonPixmap(false);

    initActions();

    QMimeData md;
    md.setUrls(QList<QUrl>() << QUrl{"file:///" + QDir::currentPath() + "/../input/test_brute_force/simple/test.txt"});
    processMimeData(&md);
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
}

MainWindow::~MainWindow()
{
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

void MainWindow::setStartPauseButtonPixmap(bool isStarted)
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
    scene->clear();
    ui->graphicsView->setDragMode(QGraphicsView::NoDrag);

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

void MainWindow::processFile(const QString &filename)
{
    Q_ASSERT(QFileInfo(filename).isFile());

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
    // разрешим перетягивание
    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);

    ui->plainTextEdit->appendPlainText("-- ошибок не обнаружено");
    if (dd.has) {
        ui->plainTextEdit->appendPlainText("-- стоимость аренды по маршруту из исходных данных равна " +
                                           QString::number(prepared::totalCost(ds, dd)));

        // отрисуем пустые трассы
        scene->setSources(ds, dd);
        return;
    }

    AlgoBruteForce algoBruteForce(ds);
    dd = algoBruteForce.find();


    scene->setSources(ds, dd);

    ui->plainTextEdit->appendPlainText("-- стоимость аренды по созданному маршруту равна " +
                                       QString::number(prepared::totalCost(ds, dd)));
}

void MainWindow::postFromClipboardRequested()
{
    processMimeData(QGuiApplication::clipboard()->mimeData());
}
