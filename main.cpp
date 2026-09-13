#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("RS232Terminal");
    a.setApplicationName("SerialTerminal");
    MainWindow w;
    w.show();
    return a.exec();
}
