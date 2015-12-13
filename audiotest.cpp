#include "audiotest.h"

#include <QDebug>
#include "stopwatch.h"

void AudioReader::onReadyRead()
{
    qint64 readresult;

    while ((readresult = aio->read((char*)(buf.data() + bufLevel),
            buf.size() - bufLevel)) > 0)
    {
        bufLevel += readresult / sizeof(short);

        auto beginBuf = std::begin(buf);
        auto stBuf = beginBuf;
        auto enBuf = stBuf + bufLevel;

        while (stBuf < enBuf) {
            auto enMax = stBuf + batchSize;

            // If there isn't enough, done
            if (enMax > enBuf)
                break;

            Stopwatch sw;
            sw.start();
            fft->process(stBuf, enMax, result, 1/16.0);
            sw.update();

            updateLevel(stBuf, enMax);

            stBuf = enMax;

            //std::transform(std::begin(result),
            //               std::end(result),
            //               std::begin(quantizedResult),
            //               [&](float f) {
            //    return (std::min)((std::numeric_limits<Sample>::max)(),
            //            Sample(f + 0.5f));
            //});

            emit incoming(std::begin(result), FFTType::halfPoints);
        }

        if (stBuf != beginBuf) {
            bufLevel = enBuf - stBuf;
            std::copy(stBuf, enBuf, beginBuf);
        }
    }
}

AudioReader::AudioReader(QObject *parent)
    : QObject(parent)
    , ai(nullptr)
    , aio(nullptr)
    , batchRatePerSec(120)
    , batchSize(0)
    , bufLevel(0)
{
}

AudioReader::~AudioReader()
{
    if (aio)
        aio->close();
    if (ai)
        ai->stop();
}

void AudioReader::start()
{
    QList<QAudioDeviceInfo> devices =
            QAudioDeviceInfo::availableDevices(QAudio::AudioInput);

    std::for_each(devices.begin(), devices.end(),
                  [](QAudioDeviceInfo& device) {
        qDebug() << device.deviceName();
    });

    qDebug() << "Default == " << QAudioDeviceInfo::defaultInputDevice().deviceName();
    
    afmt.setSampleRate(48000);
    afmt.setChannelCount(1);
    afmt.setCodec("audio/pcm");
    afmt.setByteOrder(QAudioFormat::LittleEndian);
    afmt.setSampleType(QAudioFormat::SignedInt);
    afmt.setSampleSize(16);
    if (!afmt.isValid())
        return;

    QAudioDeviceInfo deviceinfo(QAudioDeviceInfo::defaultInputDevice());

    for (auto supportedCodec : deviceinfo.supportedCodecs())
    {
        qDebug() << "Supported codec: " << supportedCodec;
    }

    char const* sampleType;
    for (auto i : deviceinfo.supportedSampleTypes())
    {
        switch (i) {
        case QAudioFormat::Unknown:
            sampleType = "Unknown";
            break;

        case QAudioFormat::SignedInt:
            sampleType = "SignedInt";
            break;

        case QAudioFormat::UnSignedInt:
            sampleType = "UnSignedInt";
            break;

        case QAudioFormat::Float:
            sampleType = "Float";
            break;

        default:
            sampleType = "???";
            break;
        }
        qDebug() << "Supports sample type: " << sampleType;
    }

    for (auto supportedRate: deviceinfo.supportedSampleRates())
    {
        qDebug() << "Supported rate: " << supportedRate;
    }

    for (auto supportedSize : deviceinfo.supportedSampleSizes())
    {
        qDebug() << "Supported size: " << supportedSize;
    }

    if (!deviceinfo.isFormatSupported(afmt))
        afmt = deviceinfo.nearestFormat(afmt);

    QAudio::Error err;
    ai = new QAudioInput(deviceinfo, afmt, this);
    ai->setBufferSize(8192);
    ai->setNotifyInterval(16);
    qDebug() << "Actual notify interval " << ai->notifyInterval();
    connect(ai, SIGNAL(notify()), this, SLOT(onNotify()));

    batchSize = afmt.sampleRate() / batchRatePerSec;

    aio = ai->start();
    connect(aio, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    //qDebug() << "State=" << ai->state();
    err = ai->error();
    if (err)
        qDebug() << "Err=" << err;

    ai->resume();

    //qDebug() << "State=" << ai->state();
    err = ai->error();
    //qDebug() << "Err=" << err;

    fft.reset(new FFTType(afmt.sampleRate()));

    startTimer(100);
    level = 0;
    emit inputLevel(level);

}

void AudioReader::onNotify()
{
    //qDebug() << "State=" << ai->state();
}

void AudioReader::stop()
{
    aio->close();
    ai->stop();
}

void AudioReader::timerEvent(QTimerEvent *event)
{
    Sample curlevel = 0;
    std::swap(curlevel, level);
    emit inputLevel(curlevel);
}

AudioReader::Sample
AudioReader::updateLevel(Sample const* st,
                         Sample const* en)
{
    level = std::accumulate(st, en, level,
    [](Sample level, Sample sample) {
        Sample absSample = std::abs(sample);
        return (std::max)(level, absSample);
    });
    return level;
}
