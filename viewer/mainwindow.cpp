#include <QMimeData>
#include <QDebug>
#include <QFileInfo>
#include <QTemporaryFile>

#include "debugcatcher.h"
#include "sourcefilereader.h"
#include "sourceerrordetector.h"
#include "rawdata.h"
#include "prepareddata.h"
#include "patherrordetector.h"

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
    event->setAccepted(hasGoodFormat(event->mimeData()));
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!hasGoodFormat(event->mimeData())) {
        setWindowTitle("StrawberryShip");
        return;
    }

    if (event->mimeData()->urls().size() == 1) {
        QString filename{ event->mimeData()->urls().first().toLocalFile() };
        setWindowTitle("StrawberryShip @ " + filename);
        return processFile(filename);
    }

    if (event->mimeData()->hasText()) {
        setWindowTitle("StrawberryShip @ via d&d text");
        QTemporaryFile tempfile;
        tempfile.open();
        tempfile.write(event->mimeData()->text().toUtf8());
        // вернёмся в начало, чтобы можно было что-то читать (хотя тут непонятно, как это работает...)
        tempfile.seek(0);
        return processFile(tempfile.fileName());
    }
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

void MainWindow::processFile(const QString &filename)
{
    Q_ASSERT(QFileInfo(filename).isFile());

    DebugCatcher::instance()->clearWaringsCount();
    ui->plainTextEdit->clear();

    SourceFileReader reader(filename);
    SourceErrorDetector rawDetector(reader.dat());

    if (DebugCatcher::instance()->warningsCount())
        return;

    prepared::DataStatic ds(reader.dat());
    prepared::DataDynamic dd(reader.dat());

    // в ds тоже несколько ошибок проверяется...
    if (DebugCatcher::instance()->warningsCount())
        return;

    if (dd.has) {
        PathErrorDetector pathErrorDetector(ds, dd);

        if (DebugCatcher::instance()->warningsCount())
            return;
    }



    ui->plainTextEdit->appendPlainText("-- ошибок не обнаружено");
    if (dd.has)
        ui->plainTextEdit->appendPlainText("-- стоимость аренды по маршруту равна " +
                                           QString::number(prepared::totalCost(ds, dd)));
}
