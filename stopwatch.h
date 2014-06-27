#ifndef STOPWATCH_H
#define STOPWATCH_H

#include <chrono>
#include <atomic>
#include "tostring.h"

class Stopwatch
{
public:
	Stopwatch();

	void start();
	void update();
	int64_t elapsedMilliseconds();
	int64_t elapsedMicroseconds();

private:
    std::chrono::high_resolution_clock::time_point st, en;
};

template<typename T>
class PerSec
{
public:
    typedef decltype(0 + std::declval<T>()) DiffType;
    typedef std::chrono::high_resolution_clock Clock;
    typedef Clock::time_point Time;

    PerSec()
        : lastValue(0)
        , lastTime(Clock::now())
        , value(0)
    {
    }

    T signal()
    {
        return ++value;
    }

    T operator++()
    {
        return signal();
    }

    T sample()
    {
        auto now = Clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(now - lastTime).count();
        auto v = 0+value;
        if (us > 0)
        {
            auto diff = v - lastValue;
            lastValue = v;
            lastTime = now;
            auto persec = int64_t(diff) * 1000000 / us;
            return persec;
        }
        return 0;
    }

    std::string operator()()
    {
        return ToString(value, " ", sample(), "/sec");
    }

    DiffType lastValue;
    Time lastTime;
    std::atomic<T> value;
};

#endif // STOPWATCH_H
