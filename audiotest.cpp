#include "audiotest.h"

#include <QDebug>
#include "stopwatch.h"

void AudioTest::onNotify()
{
    qint64 readresult;
    while ((aio->bytesAvailable() >= sizeof(buf))
           && (readresult = aio->read((char*)buf, sizeof(buf))) > 0)
    {
//        qDebug() << readresult << " <- read result";
//        qDebug() << "processing " << readresult << " samples";

        auto stBuf = std::begin(buf);
        auto enBuf = stBuf + readresult;

        Stopwatch sw;
        sw.start();
        fft.process(stBuf, enBuf, std::begin(result));
        sw.update();

        std::transform(std::begin(result), std::end(result), std::begin(quantizedResult),
        [&](float &f)
        {
            return int16_t(f * 16.0f + 0.5f);
        });

        emit incoming(quantizedResult, FFTType::halfPoints);

        qDebug() << sw.elapsedMicroseconds() << " microseconds";
    }
}

AudioTest::AudioTest(QObject *parent)
    : QObject(parent)
    , fft(44100)
    , ai(nullptr)
    , aio(nullptr)
{
}

AudioTest::~AudioTest()
{
}

void AudioTest::start()
{
    afmt.setSampleRate(44100);
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

    //delete ai;
    QAudio::Error err;
    ai = new QAudioInput(afmt, this);
    ai->setBufferSize(44100/4);
    ai->setNotifyInterval(16);
    aio = ai->start();

    aio->connect(aio, SIGNAL(readyRead()), this, SLOT(onNotify()));

    qDebug() << "State=" << ai->state();
    err = ai->error();
    qDebug() << "Err=" << err;

}

void AudioTest::stop()
{
    aio->close();
    ai->stop();
}
