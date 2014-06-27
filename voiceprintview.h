#ifndef VOICEPRINTVIEW_H
#define VOICEPRINTVIEW_H

#include <QWidget>

class VoicePrintView : public QWidget
{
    Q_OBJECT
public:
    // Width represents the number of points
    // Length represents the number of lines of scrollback
    explicit VoicePrintView(int width = 0, int length = 0, QWidget *parent = 0);

    void deliverSamples(int16_t const *data, int count);

protected:
    void paintEvent(QPaintEvent *pe);

private:
    QImage img;
    int width, length, place;

signals:


public slots:


};

#endif // VOICEPRINTVIEW_H
