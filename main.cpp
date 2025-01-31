#include "mainwindow.h"
// #include "starPromptWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFile qssFile(":/qss/flatwhite.qss");
    if(qssFile.open(QFile::ReadOnly)){
        QString qss = QLatin1String(qssFile.readAll());
        qApp->setStyleSheet(qss);
        qssFile.close();
    }
    MainWindow w;
    // starPromptWindow star;
    w.setWindowIcon(QIcon("./icon.ico"));
    w.show();
    return a.exec();
}
