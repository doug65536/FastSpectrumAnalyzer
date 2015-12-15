#include "fft.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <memory>
#include <limits>

#include "stopwatch.h"
#include "asserts.h"

static double worstError0, worstError1, worstError2;

//template<typename T, int logPoints>
//int64_t testffttype(FFT<T, logPoints> &fft, bool fulltest = false)
//{
//    typedef T Real;
////    std::array<Real, 65536> input;
//    std::vector<Real> result;
//
//    int64_t microseconds = 0;
//    int64_t bestMicroseconds = std::numeric_limits<int64_t>::max();
//    int64_t totalMicroseconds = 0;
//    int64_t totalCount = 0;
//
//    // Exhaustive brute force test that mixes a pair of waves with
//    // different amplitudes at two different nearby frequencies,
//    // for halfPoints points - 2. Error and worst case error are measured.
//
//    for (auto chk = 1, e = fft.getHalfPoints()-2; chk < e; ++chk)
//    {
//        auto freq = fft.getFrequency(chk);
//        auto freq2 = fft.getFrequency(chk+2);
//
//        Stopwatch sw;
//        sw.start();
//        fft.selfTestIn(freq, 1, false);
//        fft.selfTestIn(freq2, 0.001, true);
//        fft.process(std::back_inserter(result));
//        sw.update();
//        microseconds = sw.elapsedMicroseconds();
//        totalMicroseconds += microseconds;
//        ++totalCount;
//        if (bestMicroseconds > microseconds)
//            bestMicroseconds = microseconds;
//
// //        for (std::size_t i = 0, e = input.size(); i != e; ++i)
////        {
////            input[i] = Real(sin(2 * 3.14159265358979323 * freq * i / fft.maxFreq())
////                            + 0.001 * sin(2 * 3.14159265358979323 * freq2 * i / fft.maxFreq()));
////        }
//
////        fft.process(input.begin(), input.end(), std::back_inserter(result));
//
//        auto dumpResult = [&]
//        {
//            for (auto i = result.begin() + std::max(chk - 3, 0),
//                 e = result.begin() + std::min(chk + 3, fft.getPoints());
//                 i != e; ++i)
//                std::cout << (i - result.begin()) << ": " << std::setprecision(16) << *i << std::endl;
//            std::cout << std::endl;
//        };
//
//        for (auto i = 0, e = fft.getHalfPoints(); i < e; ++i)
//        {
//            // If not full test, check the first 100 values
//            if (i >= 50 && !fulltest)
//            {
//                // Shortcuts
//
//                // Jump to 100 points before the check frequency
//                // or do nothing if there aren't over 100 points
//                // after i before chk
//                if (i < chk - 50)
//                {
////                    std::cout << "Cheating from " << i << " to " << (chk - 100) << std::endl;
//                    i = chk - 50;
//                }
//
//                // Jump to 100 points before the end if after both
//                // check points
//                if (i > chk + 2 + 50 && i < e - 100)
//                {
////                    std::cout << "Cheating from " << i << " to " << (e - 100) << std::endl;
//                    i = e - 50;
//                }
//            }
//
//            double error0 = std::abs(0.0 - result[i]);
//            double error1 = std::abs(1.0 - result[i]);
//            double error2 = std::abs(0.001 - result[i]);
//
//            if (i == chk)
//            {
//                worstError1 = std::max(worstError1, error1);
//                if (!AssertBreakForce(error1 < 0.0015))
//                    dumpResult();
//            }
//            else if (i == chk + 2)
//            {
//                worstError2 = std::max(worstError2, error2);
//                if (!AssertBreakForce(error2 < 0.0002))
//                    // float, logPoints=15, i=12186, 0.0006371234194375575
//                    dumpResult();
//            }
//            else
//            {
//                worstError0 = std::max(worstError0, error0);
//                if (!AssertBreakForce(error0 < 0.00075))
//                    dumpResult();
//            }
//        }
//
//        result.clear();
//    }
//
//    std::cout << "Worst error 0: " << worstError0 << std::endl;
//    std::cout << "Worst error 1: " << worstError1 << std::endl;
//    std::cout << "Worst error 2: " << worstError2 << std::endl;
//    std::cout << "Best microseconds: " << bestMicroseconds << std::endl;
//
//    return totalMicroseconds / totalCount;
//}

//template<typename T, int points>
//class TestFFT
//{
//public:
//    static void test()
//    {
//        // Recursively expand and call templates of smaller point count
//        // if the point count is > 4
//        std::conditional<(points > 4), TestFFT<T, points-1>, Empty>::type::test();
//
//        auto fft = std::make_shared<FFT<T, points>>(44100);
//
//        std::cout << "Size is " << (sizeof(*fft)) << std::endl;
//
//        auto avg = testffttype(*fft);
//        std::cout << (1<<points) << " point: " << avg << " us" << std::endl;
//    }
//
//private:
//    class Empty
//    {
//    public:
//        static void test()
//        {
//        }
//    };
//};
//
//void testfft()
//{
//    // 256 point is about 5 microseconds
//    // 1024 point is about 21 microseconds
//    // 2048 point is about 49 microseconds
//    // 4096 point is about 141 microseconds
//    // 8192 point is about 332 microseconds
//    // 16384 point is about 837 microseconds
//    // 32768 point is about 2214 microseconds
//    // 65536 point is about 5233 microseconds
//    std::cout << "===================" << std::endl;
//    std::cout << "double" << std::endl;
//    std::cout << "===================" << std::endl;
//    TestFFT<double, 12>::test();
//    TestFFT<float, 12>::test();
//}
