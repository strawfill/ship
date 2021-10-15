#include "mainwindow.h"

#include <QApplication>

#include "sourcefilereader.h"

int main(int argc, char *argv[])
{
    SourceFileReader reader("../in.txt");
    reader.print();
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
