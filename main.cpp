#include <QApplication>
#include <QFontDatabase>

#include "mainwindow.h"

#include <QDebug>

//#define TTT1

#if 1
#include "movestopathconverter.h"
#include "prepareddata.h"
#include <QElapsedTimer>

#endif

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
    //for (int i = 0; i < 900; ++i) {
    //    qDebug() << qrand()%999 << qrand()%999 << qrand()%999 << qrand()%999 << qrand()%30+20;
    //}
    //return 0;

    //qsrand(4);
#ifdef TTT1
    test();
#endif
    QApplication a(argc, argv);

#if 0
    {
        for (int i = 0; i < 100; ++i) {
            int temp = 2*i;
            qlonglong t = 1;
            while (temp) {
                t *= temp--;
            }
            t /= 1LL << i;
            qDebug() << "r" << i << t;
        }


        return 0;
    }
#endif
#if 0
    enum { size=1000, size2 = 2 * size };
    prepared::DataStatic ds;
    ds.handlers.append(prepared::Handler("h", 10, 50000, 10, 30000));
    ds.shooters.append(prepared::Shooter("s", 10, 30000));
    for (int i = 0; i < size; ++i) {
        ds.tracs.append(raw::Trac{qrand()%100+i,qrand()%100+i,qrand()%100+i,qrand()%100+i,qrand()%10+3});
    }
    MovesToPathConverter converter(ds);
    converter.setShips(ds.handlers.first(), ds.shooters.first());

    std::vector<int> hplaces;
    hplaces.resize(size2);
    std::vector<int> splaces;
    splaces.resize(size);
    ShipMovesVector hmoves;
    hmoves.resize(size2);
    ShipMovesVector smoves;
    smoves.resize(size);

    for (int i = 0; i < size2; ++i)
        hplaces.at(i) = i/2;

    for (int i = 0; i < size; ++i)
        splaces.at(i) = i;

    for (int i = 0; i < size2; ++i) {
        hmoves[i] = ShipMove{short(hplaces.at(i)), bool(qrand()%2)};
    }

    for (int i = 0; i < size; ++i) {
        smoves[i] = ShipMove{short(hplaces.at(i)), bool(qrand()%2)};
    }

    qint64 s{}, res{};
    QElapsedTimer tm; tm.start();

    for (int i = 0; i < 1500; ++i) {
        //smoves[500].isP1Start = qrand()%2;
        s += converter.calculateHours(hmoves, smoves);
    }
    res = tm.nsecsElapsed();

    qDebug() << "total" << res / 1e6 << "ms (" << s << ")";

    return 0;


#endif

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
