#include "voiceprintview.h"
#include <QPainter>

VoicePrintView::VoicePrintView(int width, int length, QWidget *parent)
    : QWidget(parent)
    , img(new QImage(width, length, QImage::Format_RGBX8888))
    , width(width)
    , length(length)
    , place(0)
{
}

void VoicePrintView::deliverSamples(const int16_t *data, int count)
{
    uchar *line = img->scanLine(place++);
    place &= place < length ? -1 : 0;
    for (int i = 0; i < count; ++i)
    {
        line[i*4] = data[i] >> 8;
        line[i*4+1] = data[i] >> 8;
        line[i*4+2] = data[i] >> 8;
        line[i*4+3] = 0xFF;
    }
    update(QRect(0, place, width, 1));
}

void VoicePrintView::paintEvent(QPaintEvent *pe)
{
    // Image is drawn in two halves:
    //
    QPainter *qp;
    qp = new QPainter(this);
    qp->drawImage(QPoint(0,0), *img);
    qp->end();
}
