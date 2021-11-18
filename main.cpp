#include <QApplication>
#include <QFontDatabase>

#include "mainwindow.h"

#include <QDebug>


#if 1
#include "movestopathconverter.h"
#include "prepareddata.h"
#include <QElapsedTimer>

#endif


int main(int argc, char *argv[])
{
#if 0
    int f{250};
    int d{250};
    auto val = [=](int m = 1){ return f*m + qrand()%d; };
    for (int i = 0; i < 1000; ++i) {
        if (i % 8 == 0)
            qDebug() << val() << val() << val() << val() << qrand()%30+20;
        else if (i % 8 == 1)
            qDebug() << -val() << val() << -val() << val() << qrand()%30+20;
        else if (i % 8 == 2)
            qDebug() << val() << -val() << val() << -val() << qrand()%30+20;
        else if (i % 8 == 3)
            qDebug() << -val() << -val() << -val() << -val() << qrand()%30+20;
        else if (i % 8 == 4)
            qDebug() << val() << val(2) << val(2) << val(2) << qrand()%30+20;
        else if (i % 8 == 5)
            qDebug() << -val(2) << val(2) << -val(2) << val(2) << qrand()%30+20;
        else if (i % 8 == 6)
            qDebug() << val(2) << -val(2) << val(2) << -val(2) << qrand()%30+20;
        else if (i % 8 == 7)
            qDebug() << -val(2) << -val(2) << -val(2) << -val(2) << qrand()%30+20;
    }
    return 0;
#endif

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
