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
    typedef FFT<float, 12> FFTType;
    FFTType fft;

    QAudioInput *ai;
    QAudioFormat afmt;
    QIODevice *aio;

    // 1/60th sec
    int16_t buf[44100/120*2];
    float result[FFTType::halfPoints];
    int16_t quantizedResult[FFTType::halfPoints];

signals:

public slots:

};

#endif // AUDIOTEST_H
