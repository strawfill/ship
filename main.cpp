#include <QApplication>
#include <QFontDatabase>

#include "mainwindow.h"

//#define TTT1
#ifdef TTT1
#include <QDebug>
#include <QElapsedTimer>
#include <QtMath>

void test()
{
    QElapsedTimer tm; tm.start();
    qlonglong sum{};


    for (qlonglong i = 0; i < 14584345LL; ++i) {
        double v{ 73. + qrand() % 1 };
        sum += qCeil(v / 10);
        //sum += qCeil(v * 0.1);
        //sum += qCeil(v) / 10;
    }

    auto el = tm.nsecsElapsed();

    qDebug() << el / 1e6 << "ms to" << sum;
}

#endif

int main(int argc, char *argv[])
{
#ifdef TTT1
    test();
#endif
    QApplication a(argc, argv);

    QCoreApplication::setApplicationName("StrawberryShip");
    QCoreApplication::setApplicationVersion("1.0");
    // да, я тут работаю...
    QCoreApplication::setOrganizationName("Informtest");


    // Красивый шрифт приложения
    int id = QFontDatabase::addApplicationFont(":/SourceCodePro-Regular.ttf");
    if(id != -1){
        QStringList font_families = QFontDatabase::applicationFontFamilies(id);
        if (!font_families.isEmpty()) {
            QFont newFont(font_families.first(), 10);
            QApplication::setFont(newFont);
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
