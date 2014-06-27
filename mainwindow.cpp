#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "voiceprintview.h"

#include <QDebug>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    vpv = new VoicePrintView(1024, 1024, this);
    setCentralWidget(vpv);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::addLine(const int16_t *data, size_t sampleCount)
{
//    auto mx = std::max_element(data, data+sampleCount);
//    auto mn = std::min_element(data, data+sampleCount);
//    qDebug() << "min sample=" << mn << " max sample=" << mx;

    vpv->deliverSamples(data, sampleCount);
}
