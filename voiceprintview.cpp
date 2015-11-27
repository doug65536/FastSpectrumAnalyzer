#include "voiceprintview.h"
#include <QPainter>
#include <QtGui>

VoicePrintView::VoicePrintView(QWidget *parent)
    : QWidget(parent)
    , place(0)
{
    img = new QImage(width(), height(), QImage::Format_RGBX8888);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
}

void VoicePrintView::deliverSamples(const std::int16_t *data, std::size_t count)
{
    QRgb* line = (QRgb*)img->scanLine(place++);
    if (place >= img->height())
        place = 0;
    // If it is too many, reduce it
    if (count > img->width())
        count = img->width();
    std::transform(data, data + count, line, [](std::int16_t const& sample) -> QRgb {
        int v = int(sample);
        int v1 = std::max(0, std::min(0xFF, v));
        int v2 = std::max(0, std::min(0xFF, v - 0xFF));
        int v3 = std::max(0, std::min(0xFF, 0xFF-v2));
        // why backwards? :(
        QRgb result = /*!v2
                ? */QColor::fromRgb(v1, v1, v1).rgb()/*
                : QColor::fromRgb(v3, v3, 255).rgb()*/;

        return result;
    });
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

void VoicePrintView::resizeEvent(QResizeEvent *re)
{
    QSize viewSize = re->size();
    QSize imgSize = img->size();
    int heightDiff;

    if (viewSize.height() != imgSize.height() ||
            viewSize.width() != imgSize.width())
    {
        heightDiff = viewSize.height() - imgSize.height();
        place += heightDiff;
        while (place < 0 && viewSize.height())
            place += viewSize.height();
        if (place > viewSize.height())
            place = 0;
        // Keep bottom left part of old image
        QImage *newimage = new QImage(
                    img->copy(0, imgSize.height() - viewSize.height(),
                              viewSize.width(), viewSize.height()));
        delete img;
        img = newimage;

        if (place >= viewSize.height())
            place = 0;
    }
}
