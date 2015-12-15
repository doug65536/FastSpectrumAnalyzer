#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "voiceprintview.h"
#include "audiotest.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:

private:
    void timerEvent(QTimerEvent *event);

    AudioReader *audioReader;
    Ui::MainWindow *ui;
    VoicePrintView *vpv;
};

#endif // MAINWINDOW_H
