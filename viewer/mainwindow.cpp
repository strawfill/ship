#include <QClipboard>
#include <QDebug>
#include <QFileInfo>
#include <QMimeData>
#include <QTemporaryFile>

#include "debugcatcher.h"
#include "graphicsviewzoomer.h"
#include "prepareddata.h"
#include "patherrordetector.h"
#include "rawdata.h"
#include "simulationscene.h"
#include "sourceerrordetector.h"
#include "sourcefilereader.h"

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new SimulationScene(this))
    , pastAction(new QAction(this))
{
    ui->setupUi(this);
    // чтобы второй был минимального размера
    ui->splitter->setSizes({10000, 1});

    connect(DebugCatcher::instance(), &DebugCatcher::messageRecieved, ui->plainTextEdit, &QPlainTextEdit::appendPlainText);

    connect(ui->tb_start, &QToolButton::clicked, scene, &SimulationScene::startSimulation);
    connect(ui->tb_pause, &QToolButton::clicked, scene, &SimulationScene::pauseSimulation);
    connect(ui->tb_stop, &QToolButton::clicked, scene, &SimulationScene::stopSimulation);
    connect(ui->doubleSpinBox_speed, QOverload<double>::of(&QDoubleSpinBox::valueChanged), scene, &SimulationScene::setSimulationSpeed);
    scene->setSimulationSpeed(ui->doubleSpinBox_speed->value());

    connect(scene, &SimulationScene::simulationTimeChanged, ui->label_time, &QLabel::setText);

    ui->graphicsView->setScene(scene->getScene());


    ui->graphicsView->setDragMode(QGraphicsView::ScrollHandDrag);
    ui->graphicsView->scale(1, -1);

    new GraphicsViewZoomer(ui->graphicsView);

    addAction(pastAction);
    pastAction->setShortcut(QKeySequence::Paste);
    connect(pastAction, &QAction::triggered, this, &MainWindow::postFromClipboardRequested);
}

MainWindow::~MainWindow()
{
    delete ui;
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
    const QString filename{ data->urls().first().toLocalFile() };
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

    if (!data)
        return;

    if (!hasGoodFormat(data)) {
        setWindowTitle("StrawberryShip");
        return;
    }

    if (data->urls().size() == 1) {
        QString filename{ data->urls().first().toLocalFile() };
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

    DebugCatcher::instance()->clearWaringsCount();
    ui->plainTextEdit->clear();

    SourceFileReader reader(filename);
    SourceErrorDetector rawDetector(reader.dat());

    if (DebugCatcher::instance()->warningsCount())
        return;

    prepared::DataStatic sd(reader.dat());
    prepared::DataDynamic dd(reader.dat());

    // в ds тоже несколько ошибок проверяется...
    if (DebugCatcher::instance()->warningsCount())
        return;

    if (dd.has) {
        PathErrorDetector pathErrorDetector(sd, dd);

        if (DebugCatcher::instance()->warningsCount())
            return;
    }

    // и мы можем уже задать текущие данные для графической симуляции
    // не важно, есть ли path, мы просто отрисуем трассы тогда
    scene->setSources(sd, dd);

    ui->plainTextEdit->appendPlainText("-- ошибок не обнаружено");
    if (dd.has) {
        ui->plainTextEdit->appendPlainText("-- стоимость аренды по маршруту из исходных данных равна " +
                                           QString::number(prepared::totalCost(sd, dd)));
        return;
    }
}

void MainWindow::postFromClipboardRequested()
{
    processMimeData(QGuiApplication::clipboard()->mimeData());
}
