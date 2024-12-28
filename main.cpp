#include "mainwindow.h"
#include "starPromptWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    starPromptWindow star;
    w.setWindowIcon(QIcon("./icon.ico"));
    w.show();
    return a.exec();
}
