#ifndef VOICEPRINTVIEW_H
#define VOICEPRINTVIEW_H

#include <QWidget>

class VoicePrintView : public QWidget
{
    Q_OBJECT
public:
    // Width represents the number of points
    // Length represents the number of lines of scrollback
    VoicePrintView(QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *pe);
    void resizeEvent(QResizeEvent *re);

private:
    QImage *img;
    int place;

signals:

public slots:
    void deliverSamples(std::int16_t const *data, std::size_t count);
};

#endif // VOICEPRINTVIEW_H
