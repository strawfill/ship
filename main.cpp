#include <QApplication>
#include <QFontDatabase>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
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
