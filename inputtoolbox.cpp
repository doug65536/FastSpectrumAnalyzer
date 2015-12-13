#include "inputtoolbox.h"
#include "ui_inputtoolbox.h"

InputToolBox::InputToolBox(QWidget *parent) :
    QToolBox(parent),
    ui(new Ui::InputToolBox)
{
    ui->setupUi(this);
}

InputToolBox::~InputToolBox()
{
    delete ui;
}
