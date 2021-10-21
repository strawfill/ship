#include <QMimeData>
#include <QDebug>
#include <QFileInfo>

#include "debugcatcher.h"
#include "sourcefilereader.h"
#include "sourceerrordetector.h"
#include "rawdata.h"

#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // чтобы второй был минимального размера
    ui->splitter->setSizes({10000, 1});

    connect(DebugCatcher::instance(), &DebugCatcher::messageRecieved, ui->plainTextEdit, &QPlainTextEdit::appendPlainText);
}

MainWindow::~MainWindow()
{
    delete ui;
}



void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    event->setAccepted(hasGoodFile(event->mimeData()));
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!hasGoodFile(event->mimeData())) {
        setWindowTitle("StrawberryShip");
        return;
    }

    Q_ASSERT(event->mimeData()->urls().size());
    processFile(event->mimeData()->urls().first().toLocalFile());
}

bool MainWindow::hasGoodFile(const QMimeData *data)
{
    if (!data)
        return false;
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

void MainWindow::processFile(const QString &filename)
{
    Q_ASSERT(QFileInfo(filename).isFile());

    setWindowTitle("StrawberryShip @ " + filename);

    DebugCatcher::instance()->clearWaringsCount();
    ui->plainTextEdit->clear();

    SourceFileReader reader(filename);
    SourceErrorDetector detector(reader.dat());

    if (DebugCatcher::instance()->warningsCount())
        return;

    ui->plainTextEdit->appendPlainText("-- ошибок не обнаружено");
}
