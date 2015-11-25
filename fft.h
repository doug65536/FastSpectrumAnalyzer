#ifndef FFT_H
#define FFT_H

#include <array>
#include <cmath>
#include <map>
#include <iterator>
#include <type_traits>
#include <stdexcept>
#include <algorithm>

#include <iostream>
#include <complex>

#if 0 && defined(__GNUC__) && defined(__SSE4_2__)
#include <xmmintrin.h>
template<typename T>
class Complex
{
public:
    Complex& operator+=(Complex const& r) noexcept
    {
        vec += r.vec;
    }
    Complex& operator-=(Complex const& r) noexcept
    {
        vec -= r.vec;
    }
    Complex& operator*=(Complex const& r) noexcept
    {
        // a=((s1.rel)*(s2.rel))-((s1.img)*(s2.img));
        // b=((s1.rel)*(s2.img))+((s2.rel)*(s1.img));
        __m128d
_mm_dp_ps()
        vec2 swapped = __builtin_shuffle(r.vec, ivec2{1, 0});
    }
    Complex operator+(Complex const& r) const noexcept;
    Complex operator-(Complex const& r) const noexcept;
    Complex operator*(Complex const& r) const noexcept;

    T real() const noexcept;
    T imag() const noexcept;

    void real(T value) noexcept;
    void imag(T value) noexcept;

private:
    typedef double __attribute__((vector_size(16))) vec2;
    typedef __int64_t __attribute__((vector_size(16))) ivec2;

    vec2 vec;
};
#else
template<typename T>
using Complex = std::complex<T>;
#endif

template<typename T>
struct Quantizer;

template<>
class Quantizer<float>
{
public:
    static constexpr float value = 256;
};

template<>
class Quantizer<double>
{
public:
    static constexpr double value = 16384;
};

// 134 microseconds for 4096 point fft on Core I7 Extreme @ 3.46GHz

template<typename Real, int logPoints>
class __attribute__((aligned(16))) FFT
{
public:
    typedef Real value_type;

    static constexpr int log2Points = logPoints;
    static constexpr int points = 1 << logPoints;
    static constexpr int halfPoints = 1 << (logPoints-1);

    typedef std::array<Real, halfPoints> ResultContainer;

    explicit FFT(int sampleRate) noexcept
        : sampleRate(sampleRate)
    {
        clear();

        // Generate w lookup table
        initLookupTables();
    }

    constexpr int getPoints() const noexcept
    {
        return points;
    }

    constexpr int getHalfPoints() const noexcept
    {
        return halfPoints;
    }

    Real getFrequency(Real point) const noexcept
    {
        return Real(sampleRate) * point / points;
    }

    Real intensityOf(int point, bool quantized = false) const noexcept
    {
        //return x[point].Mod() / sqrtPoints;
        // Quantize away the noise so 0 is 0 instead of some random tiny value,
        // like 1.5e-14
        // volatile attempts to force the optimizer not to optimize away the
        // quantization. works on gcc 4.8
        // 16384 was chosen from observations of output with pure exact input tones
        // to quantize the noise away while having no effect on peak result
        if (quantized)
        {
            volatile Real tmp = (x[point].mod() * normalizer + Quantizer<Real>::value);
            return tmp - Quantizer<Real>::value;
        }
        return x[point].mod() * normalizer;
    }

    int hzToPoint(int freq) const noexcept
    {
        return points * freq / sampleRate;
    }

    // Linear interpolation
    Real hzToIntensity(Real freq) const noexcept
    {
        auto subPoint = points * freq / sampleRate;
        auto flrPoint = std::floor(subPoint);
        auto frac = subPoint - flrPoint;
        auto intPoint = int(flrPoint);

        if (frac == 0)
            return intensityOf(intPoint);

        auto st = intensityOf(intPoint);
        auto en = intensityOf(intPoint+1);
        auto delta = en - st;

        return st + delta * frac;
    }

    int maxFreq() const noexcept
    {
        return sampleRate;
    }

    void transform()
    {
        int step = 1;
        for (int level = 0; level < logPoints; ++level)
        {
            int increm = step + step;
            for (int j = 0; j < step; ++j)
            {
                auto u = w[level][j];
                for (int i = j; i < points; i += increm)
                {
                    auto a = x[i];
                    auto b = x[i+step];
                    auto t = b * u;
                    b = a - t;
                    a += t;
                    x[i] = a;
                    x[i+step] = b;
                }
            }
            step += step;
        }
//        std::transform(x.data(), x.data() + halfPoints, output.data(),
//                       [](Complex<Real>& value) {
//            return std::abs(value) * normalizer;
//        });
        //Complex<Real>::mod(output.data(), x.data(), halfPoints, normalizer);
    }

    void process()
    {
        transform();
        std::transform(x.data(), x.data() + halfPoints, output.data(),
                       [](Complex<Real>& value) {
            return std::abs(value) * normalizer;
        });
        //Complex<Real>::mod(output.data(), x.data(), halfPoints, normalizer);
    }

    // Outputs halfPoints result values
    template<typename OutputIt,
             typename OV = typename std::iterator_traits<OutputIt>::value_type,
             typename = typename std::is_convertible<Real, OV>::type>
    void process(OutputIt out) noexcept
    {
        process();
        std::copy(std::begin(output), std::end(output), out);
    }

    template<typename InputIt, typename U = Real,
             typename IV = typename std::iterator_traits<InputIt>::value_type,
             typename = typename std::enable_if<std::is_convertible<IV, Real>::value>::type,
             typename OutputIt,
             typename OV = typename std::iterator_traits<OutputIt>::value_type/*,
             typename = typename std::enable_if<std::is_convertible<Real, OV>::value>::type*/>
    void process(InputIt st, InputIt en, OutputIt out, U scale = Real(1)) noexcept
    {
        copyIn(st, en, scale);
        process(out);
    }

    void put(int i, Real val) noexcept
    {
        x[bitrev[i]] = Complex<Real>(val);
    }

    void add(int i, Real val) noexcept
    {
        x[bitrev[i]] += Complex<Real>(val);
    }

    void clear() noexcept
    {
        tape.fill(0);
    }

    // Process a pure sine wave at the specified frequency
    void selfTestIn(Real freq, Real amplitude = Real(1), bool mix = false)
    {
        auto step = Real(2) * Real(3.14159265358979323) * freq / sampleRate;
        if (mix)
        {
            for (std::size_t i = 0, e = tape.size(); i != e; ++i)
                add(i, amplitude * Real(std::sin(step * i)));
        }
        else
        {
            for (std::size_t i = 0, e = tape.size(); i != e; ++i)
                put(i, amplitude * Real(std::sin(step * i)));
        }
    }

    template<typename InputIt, typename U = Real,
             typename V = typename std::iterator_traits<InputIt>::value_type,
             typename = typename std::is_convertible<V, Real>::type>
    void copyIn(InputIt st, InputIt en, U scale = Real(1)) noexcept
    {
        auto n = std::distance(st, en);

        if (n > tape.size())
        {
            // Discard all but enough for a full buffer
            st = en - tape.size();
            n = tape.size();
        }
        else if (n < tape.size())
        {
            // Scroll tape if not filling it
            std::copy(tape.begin() + n, tape.end(), tape.begin());
        }

        // Copy data into tape, applying scale factor
        std::transform(st, en, tape.end() - n, [scale](V i) -> Real
        {
            return Real(i * scale);
        });

        // Scatter the data into x using bitrev
        for (auto i = 0, e = points; i != e; ++i)
            put(i, tape[i]);
    }

    void debugDumpTape() const noexcept
    {
        std::cout << "tape" << std::endl;
        for (auto& wtf : tape)
            std::cout << wtf << std::endl;
    }

    Real *getResultBuffer() noexcept
    {
        return output.data();
    }

    template<typename OutputIt,
             typename V = typename std::iterator_traits<OutputIt>::value_type,
             typename = typename std::enable_if<std::is_convertible<Real,V>::value>::type>
    void rebuildInput(OutputIt out)
    {
        std::array<Real, halfPoints> result;
        result.fill(Real(0));
        // To maximize floating point precision, the frequencies are
        // sorted by amplitude, with the lowest amplitudes being mixed
        // in before the highest amplitudes
        std::multimap<Real, int> amplitudes;
        for (auto i = 0, e = halfPoints; i != e; ++i)
            // Skip zero amplitudes
            if (output[i] < Real(-0.0) || output[i] > Real(0.0))
                amplitudes.emplace(output[i], i);
        // Start at lowest amplitudes, end at highest amplitudes
        auto basestep =  2 * 3.14159265358979323 / sampleRate;
        for (auto i = amplitudes.begin(), e = amplitudes.end(); i != e; ++i)
        {
            auto point = i->second;
            auto amplitude = i->first;
            // Find frequency for this point
            auto freq = getFrequency(point);
            //
            std::cout << "Frequency: " << freq << ", Amplitude: " << amplitude << std::endl;
            // Mix a sine wave at that amplitude
            auto step = basestep * freq;
            for (std::size_t r = 0, e = result.size(); r != e; ++r)
                result[r] += Real(amplitude * sin(step * r));
        }
        std::copy(result.begin(), result.end(), out);
    }

    void quantizeOutput(Real factor)
    {
        std::transform(output.begin(), output.end(), output.begin(), [factor](Real v)
        {
            return v + factor;
        });
        std::transform(output.begin(), output.end(), output.begin(), [factor](Real v)
        {
            return v - factor;
        });
    }

    ResultContainer makeResultContainer()
    {
        return ResultContainer();
    }

private:
    void initLookupTables()
    {
        Real l2 = 2;
        for (auto level = 0; level < logPoints; ++level)
        {
            for (auto i = 0; i < points; ++i)
            {
                Real re = std::cos(Real(2) * Real(3.14159265358979323) * i / l2);
                Real im = -std::sin(Real(2) * Real(3.14159265358979323) * i / l2);
                w[level][i] = Complex<Real>(re, im);
            }
            l2 *= 2;
        }

        // Generate bit reversed index lookup table
        int rev = 0;
        for (auto i = 0; i < points - 1; ++i)
        {
            bitrev[i] = rev;
            int mask = halfPoints;

            while (rev >= mask)
            {
                rev -= mask;
                mask >>= 1;
            }
            rev += mask;
        }
        bitrev[points-1] = points-1;
    }

    static constexpr Real sqrtPointsLookup[24] = {
        1,
        1.4142135623730951,
        2,
        2.8284271247461903,
        4,
        5.656854249492381,
        8,
        11.313708498984761,
        16,
        22.627416997969522,
        32,
        45.254833995939045,
        64,
        90.50966799187809,
        128,
        181.01933598375618,
        256,
        362.03867196751236,
        512,
        724.0773439350247,
        1024,
        1448.1546878700494,
        2048,
        2896.309375740099
    };

    static constexpr Real normalizerLookup[24] = {
        2,
        1,
        0.5,
        0.24999999999999997,
        0.125,
        0.06249999999999999,
        0.03125,
        0.015624999999999998,
        0.0078125,
        0.0039062499999999996,
        0.001953125,
        0.0009765624999999999,
        0.00048828125,
        0.00024414062499999997,
        0.0001220703125,
        0.00006103515624999999,
        0.000030517578125,
        0.000015258789062499998,
        0.00000762939453125,
        0.0000038146972656249996,
        0.0000019073486328125,
        9.536743164062499e-7,
        4.76837158203125e-7,
        2.3841857910156247e-7,
    };

    static constexpr Real sqrtPoints = sqrtPointsLookup[logPoints];
    //static constexpr Real normalizer = 1.0 / (1<<(logPoints / 2 - 1)) / sqrtPoints;
    static constexpr Real normalizer = normalizerLookup[logPoints];

    // No dynamic allocation!
    // All memory buffers are fixed size and part of this object

    //typedef Compl
    typedef std::array<Real, points> TapeContainer;
    typedef std::array<int, points> BitRevContainer;
    typedef std::array<Complex<Real>, points> XContainer;
    typedef std::array<XContainer, logPoints> WContainer;

    // Gaps
    TapeContainer tape;
    __m128 gap0;
    BitRevContainer bitrev;
    __m128 gap1;
    XContainer x;
    __m128 gap2;
    WContainer w;
    __m128 gap3;
    ResultContainer output;

    int sampleRate;
};

#endif // FFT_H


