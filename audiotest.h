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

public:
    typedef std::int16_t Sample;

signals:
    void incoming(std::int16_t const* data, std::size_t sampleCount);
    void inputLevel(int level);

public slots:
    void onNotify();
    void onReadyRead();
    void setGain(int gain);
    void setBatchRate(int ratePerSec);

public:
    explicit AudioReader(QObject *parent = nullptr);

    ~AudioReader();

    void start();
    void stop();

    int sampleRate() const;

    int getBatchRate() const noexcept;


private:
    void timerEvent(QTimerEvent * event);

    Sample updateLevel(Sample const* st, Sample const* en);

    typedef FFT<double, 13> FFTType;
    std::unique_ptr<FFTType> fft;

    QAudioInput *ai;
    QAudioFormat afmt;
    QIODevice *aio;

    int batchRatePerSec;
    int batchSize;
    Sample level;
    float inverseGain;

    std::array<Sample,3200> buf;
    int bufLevel;
    Sample result[FFTType::halfPoints];
    Sample quantizedResult[FFTType::halfPoints];

signals:

public slots:

};

#endif // AUDIOTEST_H
