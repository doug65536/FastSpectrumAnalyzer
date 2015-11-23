#include "mainwindow.h"
#include <QApplication>

#include "audiotest.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    AudioTest *test = new AudioTest(&a);
    test->start();

    MainWindow w;

    w.connect(test, SIGNAL(incoming(const int16_t*, size_t)),
              &w, SLOT(addLine(const int16_t*, size_t)));
    w.show();

    return a.exec();
}
