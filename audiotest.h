#ifndef AUDIOTEST_H
#define AUDIOTEST_H

#include <memory>
#include <array>
#include <QObject>
#include <QAudioInput>
#include "fft.h"

class AudioReader : public QObject
{
    Q_OBJECT

signals:
    void incoming(std::int16_t const* data, std::size_t sampleCount);

public slots:
    void onNotify();
    void onReadyRead();

public:
    explicit AudioReader(QObject *parent = nullptr);

    ~AudioReader();

    void start();
    void stop();

    int sampleRate() const;

    void setBatchRate(int ratePerSec) noexcept;
    int getBatchRate() const noexcept;

private:
    typedef FFT<float, 12> FFTType;
    std::unique_ptr<FFTType> fft;

    QAudioInput *ai;
    QAudioFormat afmt;
    QIODevice *aio;

    int batchRatePerSec;
    int batchSize;

    std::array<std::int16_t,3200> buf;
    int bufLevel;
    float result[FFTType::halfPoints];
    std::int16_t quantizedResult[FFTType::halfPoints];

signals:

public slots:

};

#endif // AUDIOTEST_H
