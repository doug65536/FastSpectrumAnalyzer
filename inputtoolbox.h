#ifndef INPUTTOOLBOX_H
#define INPUTTOOLBOX_H

#include <QToolBox>

namespace Ui {
class InputToolBox;
}

class InputToolBox : public QToolBox
{
    Q_OBJECT

public:
    explicit InputToolBox(QWidget *parent = 0);
    ~InputToolBox();

private:
    Ui::InputToolBox *ui;
};

#endif // INPUTTOOLBOX_H
