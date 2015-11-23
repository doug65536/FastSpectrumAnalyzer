#include "voiceprintview.h"
#include <QPainter>

VoicePrintView::VoicePrintView(int width, int length, QWidget *parent)
    : QWidget(parent)
    , img(new QImage(width, length, QImage::Format_RGBX8888))
    , width(width)
    , length(length)
    , place(0)
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void VoicePrintView::deliverSamples(const int16_t *data, int count)
{
    QRgb* line = (QRgb*)img->scanLine(place++);
    if (place >= length)
        place = 0;
    std::transform(data, data + count, line, [](QRgb const& pixel) {
        int v = std::min(int(0xFF), int(pixel >> 8) & 0xFF);
        return QColor::fromRgb(v, v, v).rgb();
    });
    //update(QRect(0, place, width, 1));
    update();
}

void VoicePrintView::paintEvent(QPaintEvent *pe)
{
    // Image is drawn in two halves:
    // place is needs to be at the bottom
    //
    // +--------+<-- place
    // | bottom |
    // |--------|<-- 0
    // |  top   |
    // +--------+<-- place - 1
    //
    // bottom:
    //  position: 0
    //  start: place
    //  height: imgHeight - place
    // top:
    //  position: imgHeight - place
    //  start: 0
    //  height: place
    
    QPainter *qp;
    qp = new QPainter(this);

    int rest = img->height() - place;
    int width = img->width();
    int height = img->height();
    
    // Draw the stuff before place
    if (place != 0)
    {
        qp->drawImage(0, 0, *img, 
                      0, place,
                      width, rest);
    }
    qp->drawImage(0, rest, *img,
                  0, 0,
                  width, place);
    
    //qp->drawImage(QPoint(0,0), *img);
    qp->end();
}
