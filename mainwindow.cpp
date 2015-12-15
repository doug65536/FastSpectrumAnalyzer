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

    audioReader = new AudioReader(this);

    //vpv = new VoicePrintView(this);
    //setCentralWidget(vpv);

    vpv = ui->viewArea;

    connect(audioReader, SIGNAL(incoming(std::int16_t const*,std::size_t)),
            vpv, SLOT(deliverSamples(std::int16_t const*,std::size_t)));

    connect(audioReader, SIGNAL(inputLevel(int)),
            ui->inputLevel, SLOT(setValue(int)));

    connect(ui->inputGain, SIGNAL(valueChanged(int)),
            audioReader, SLOT(setGain(int)));

    connect(ui->speedSlider, SIGNAL(valueChanged(int)),
            audioReader, SLOT(setBatchRate(int)));

    startTimer(8, Qt::PreciseTimer);

    audioReader->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    ui->viewArea->repaint();
}
