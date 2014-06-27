#ifndef AUDIOTEST_H
#define AUDIOTEST_H

#include <QObject>
#include <QAudioInput>
#include "fft.h"

class AudioTest : public QObject
{
    Q_OBJECT

signals:
    void incoming(int16_t const* data, size_t sampleCount);

public slots:
    void onNotify();

public:
    explicit AudioTest(QObject *parent = nullptr);

    ~AudioTest();

    void start();
    void stop();

private:
    FFT<float, 10> fft;

    QAudioInput *ai;
    QAudioFormat afmt;
    QIODevice *aio;

    // 1/60th sec
    int16_t buf[44100/120*2];
    float result[44100/120];

signals:

public slots:

};

#endif // AUDIOTEST_H
