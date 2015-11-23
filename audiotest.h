#ifndef AUDIOTEST_H
#define AUDIOTEST_H

#include <memory>
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
    void onReadyRead();

public:
    explicit AudioTest(QObject *parent = nullptr);

    ~AudioTest();

    void start();
    void stop();

    int sampleRate() const;

private:
    typedef FFT<float, 14> FFTType;
    std::unique_ptr<FFTType> fft;

    QAudioInput *ai;
    QAudioFormat afmt;
    QIODevice *aio;

    int rate;

    // 1/60th sec
    int16_t buf[44100/120*2];
    float result[FFTType::halfPoints];
    int16_t quantizedResult[FFTType::halfPoints];

signals:

public slots:

};

#endif // AUDIOTEST_H
