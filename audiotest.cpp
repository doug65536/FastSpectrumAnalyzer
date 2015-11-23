#include "audiotest.h"

#include <QDebug>
#include "stopwatch.h"

void AudioTest::onReadyRead()
{
    qint64 available;
    qint64 readresult;
    while ((readresult = aio->read((char*)buf, sizeof(buf))) > 0)
    {
//        qDebug() << readresult << " <- read result";
//        qDebug() << "processing " << readresult << " samples";

        auto stBuf = std::begin(buf);
        auto enBuf = stBuf + (readresult / sizeof(short));

        Stopwatch sw;
        sw.start();
        fft->process(stBuf, enBuf, std::begin(result), 32.0);
        sw.update();

        std::transform(std::begin(result), std::end(result), std::begin(quantizedResult),
        [&](float &f)
        {
            return int16_t(f * 16.0f + 0.5f);
        });

        emit incoming(quantizedResult, FFTType::halfPoints >> 2);

        //qDebug() << sw.elapsedMicroseconds() << " microseconds";
    }
}

AudioTest::AudioTest(QObject *parent)
    : QObject(parent)
    , ai(nullptr)
    , aio(nullptr)
    , rate(0)
{
}

AudioTest::~AudioTest()
{
}

void AudioTest::start()
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
    if (!deviceinfo.isFormatSupported(afmt))
        deviceinfo.nearestFormat(afmt);

    QAudio::Error err;
    ai = new QAudioInput(deviceinfo, afmt, this);
    ai->setBufferSize(44100/4);
    ai->setNotifyInterval(4);
    connect(ai, SIGNAL(notify()), this, SLOT(onNotify()));

    aio = ai->start();
    connect(aio, SIGNAL(readyRead()), this, SLOT(onReadyRead()));

    //qDebug() << "State=" << ai->state();
    err = ai->error();
    //qDebug() << "Err=" << err;

    ai->resume();

    //qDebug() << "State=" << ai->state();
    err = ai->error();
    //qDebug() << "Err=" << err;

    fft.reset(new FFTType(afmt.sampleRate()));
}

void AudioTest::onNotify()
{
    //qDebug() << "State=" << ai->state();
}

void AudioTest::stop()
{
    aio->close();
    ai->stop();
}
