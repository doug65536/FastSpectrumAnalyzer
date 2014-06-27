#include "mainwindow.h"
#include <QApplication>

#include "audiotest.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

//    for (int x = 100; x < 20000; x += 500)
//    {
//        fft.selfTestIn(10000);
//        sw.start();
//        fft.process();
//        sw.update();
//        // about 506-535 microseconds on 1GHz Samsung Galaxy II X
//        // 104-177 microseconds on 1.65GHz Bobcat APU
//        qDebug() << sw.elapsedMicroseconds() << " microseconds";
//    }

    AudioTest *test = new AudioTest(&a);
    test->start();

    MainWindow w;
    w.show();

    w.connect(test, SIGNAL(incoming(const int16_t*)), &w, SLOT(addLine(const int16_t*)));

//    test->stop();
//    delete test;

//    delete test;

    return a.exec();
}
