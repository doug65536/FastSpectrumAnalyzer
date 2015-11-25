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

    vpv = new VoicePrintView(this);
    setCentralWidget(vpv);

    connect(audioReader, SIGNAL(incoming(const std::int16_t*,std::size_t)),
            vpv, SLOT(deliverSamples(const std::int16_t*,std::size_t)));

    audioReader->start();
}

MainWindow::~MainWindow()
{
    delete ui;
}
