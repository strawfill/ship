#include "mainwindow.h"

#include <QApplication>

#include "sourcefilereader.h"
#include "sourceerrordetector.h"
#include "rawdata.h"

int main(int argc, char *argv[])
{
    SourceFileReader reader("../ship/input/1/Ship3.txt");
    SourceErrorDetector detector(reader.dat());
    //reader.constDat().printToDebug();
    return 0;

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

enum class Type : bool {
   shooter,
   header,
};
