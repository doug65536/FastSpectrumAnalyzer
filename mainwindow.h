#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "voiceprintview.h"

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
    void addLine(int16_t const* data, size_t sampleCount);

private:
    Ui::MainWindow *ui;
    VoicePrintView *vpv;
};

#endif // MAINWINDOW_H
