#include "stopwatch.h"

Stopwatch::Stopwatch()
{
}

void Stopwatch::start()
{
	st = std::chrono::high_resolution_clock::now();
}

void Stopwatch::update()
{
	en = std::chrono::high_resolution_clock::now();
}

int64_t Stopwatch::elapsedMilliseconds()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(en - st).count();
}

int64_t Stopwatch::elapsedMicroseconds()
{
	return std::chrono::duration_cast<std::chrono::microseconds>(en - st).count();
}
