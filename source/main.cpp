#include "animate_packer.h"

#include <QtGui>
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AnimatePacker w;
    w.setWindowTitle("AnimatePacker4");
    w.show();
    return a.exec();
}
