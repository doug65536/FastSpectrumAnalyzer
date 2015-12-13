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

#if 1
#include <complex>
template<typename TReal>
class __attribute__((aligned(16))) Complex
{
public:
    typedef TReal Real;

    constexpr Complex() noexcept : imagPart(0.0), realPart(0.0) {}
    constexpr Complex(Real re) noexcept : imagPart(0.0), realPart(re) {}
    constexpr Complex(Real re, Real im) noexcept : imagPart(im), realPart(re) {}
    Real real() const noexcept { return realPart; }
    Real imag() const noexcept { return imagPart; }
    Complex operator +(Complex const& c) const noexcept
    {
        return Complex(realPart + c.realPart, imagPart + c.imagPart);
    }
    Complex& operator +=(Complex const& c) noexcept
    {
        realPart += c.realPart;
        imagPart += c.imagPart;
        return *this;
    }
    Complex operator -(Complex const& c) const noexcept
    {
        return Complex(realPart - c.realPart, imagPart - c.imagPart);
    }
    Complex& operator -=(Complex const& c) noexcept
    {
        realPart -= c.realPart;
        imagPart -= c.imagPart;
        return *this;
    }
    Complex operator *(Complex const& c) noexcept
    {
        return Complex(c.realPart * realPart - c.imagPart * imagPart,
                c.realPart * imagPart + c.imagPart * realPart);
    }
    Complex& operator *=(Complex const& c) noexcept
    {
        Real reT = c.realPart * realPart - c.imagPart * imagPart;
        imagPart = c.realPart * imagPart + c.imagPart * realPart;
        realPart = reT;
        return *this;
    }
    Complex operator -()
    {
        return Complex(-realPart, -imagPart);
    }

    Real mod() const
    {
        return std::sqrt(realPart * realPart + imagPart * imagPart);
    }

    template<typename OutputIt,
             typename OV = typename std::iterator_traits<OutputIt>::value_type>
    static void mod(OutputIt outputResult, Complex const *addr, size_t count, Real scale)
    {
        Real hi = Real((std::numeric_limits<OV>::max)());
        Real lo = Real((std::numeric_limits<OV>::min)());
        std::transform(addr, addr + count, outputResult, [&](Complex const& i) {
            Real a = i.mod() * scale;
            Real b = std::max(lo, a);
            Real c = std::min(hi, b);
            return c;
        });
    }

    void stream_store(Complex *address)
    {
        address->imagPart = imagPart;
        address->realPart = realPart;
    }

private:
    Real imagPart;
    Real realPart;
};

#ifdef __SSE4_1__
#include <smmintrin.h>
template<>
class __attribute__((aligned(16))) Complex<double>
{
public:
    typedef double Real;

    Complex() noexcept
    {
        mm = _mm_setzero_pd();
    }

    Complex(Real re) noexcept
    {
        mm = _mm_set_pd(re, 0.0);
    }

    Complex(Real re, Real im) noexcept
    {
        mm = _mm_set_pd(re, im);
    }

    Real real() const noexcept
    {
        return _mm_cvtsd_f64(_mm_shuffle_pd(mm, mm, _MM_SHUFFLE2(1, 1)));
    }

    Real imag() const noexcept
    {
        return _mm_cvtsd_f64(mm);
    }

    Complex operator +(Complex const& c) const noexcept
    {
        return Complex(_mm_add_pd(mm, c.mm));
    }

    Complex& operator +=(Complex const& c) noexcept
    {
        mm = _mm_add_pd(mm, c.mm);
        return *this;
    }

    Complex operator -(Complex const& c) const noexcept
    {
        return Complex(_mm_sub_pd(mm, c.mm));
    }

    Complex& operator -=(Complex const& c) noexcept
    {
        mm = _mm_sub_pd(mm, c.mm);
        return *this;
    }

    Complex operator *(Complex const& c) const noexcept
    {
        auto imagTmp = _mm_dp_pd(c.mm, _mm_shuffle_pd(mm, mm, _MM_SHUFFLE2(0, 1)), 0x31);
        // Flip sign of the imaginary part
        auto t = _mm_xor_pd(mm, _mm_set_pd(0.0, -0.0));
        auto realTmp = _mm_dp_pd(c.mm, t, 0x31);
        return Complex(_mm_shuffle_pd(imagTmp, realTmp, _MM_SHUFFLE2(0, 0)));
    }

    Complex& operator *=(Complex const& c) noexcept
    {
        auto imagTmp = _mm_shuffle_pd(mm, mm, _MM_SHUFFLE2(0, 1));
        imagTmp = _mm_dp_pd(imagTmp, c.mm, 0x31);
        // Flip sign of the imaginary part
        mm = _mm_xor_pd(mm, _mm_set_pd(0.0, -0.0));
        auto realTmp = _mm_dp_pd(mm, c.mm, 0x31);
        mm = _mm_shuffle_pd(imagTmp, realTmp, _MM_SHUFFLE2(0, 0));

//        Real reT = c.realPart * realPart - c.imagPart * imagPart;
//        imagPart = c.realPart * imagPart + c.imagPart * realPart;
//        realPart = reT;
        return *this;
    }

    Complex operator -() const
    {
        return Complex(_mm_xor_pd(mm, _mm_set_pd(-0.0, -0.0)));
    }

    Real mod() const
    {
        auto tmp = _mm_dp_pd(mm, mm, 0x31);
        return _mm_cvtsd_f64(_mm_sqrt_sd(tmp, tmp));
    }

    static void mod(double *outputResult, Complex const *addr, size_t count, Real scale)
    {
        size_t i;
        auto mmscale = _mm_set1_pd(scale);
        for (i = 0; i != count && i + 1 < count; i += 2)
        {
            auto pair0 = addr[i].mm;
            auto pair1 = addr[i + 1].mm;

            pair0 = _mm_dp_pd(pair0, pair0, 0x31);
            pair1 = _mm_dp_pd(pair1, pair1, 0x31);

            pair0 = _mm_shuffle_pd(pair0, pair1, _MM_SHUFFLE2(0, 0));

            pair0 = _mm_sqrt_pd(pair0);

            pair0 = _mm_mul_pd(pair0, mmscale);

            _mm_store_pd(outputResult + i, pair0);
        }
        for ( ; i < count; ++i)
            _mm_store_sd(outputResult + i, _mm_mul_sd(_mm_sqrt_sd(_mm_setzero_pd(),
                    _mm_dp_pd(addr[i].mm, addr[i].mm, 0x31)), mmscale));
    }

#ifdef __fuckAVX__
    static void mod(short *outputResult, Complex const *addr, size_t count, Real scale)
    {
        size_t i;
        auto mmscale = _mm_set1_pd(scale);
        auto lo = _mm_set1_pd(std::numeric_limits<std::int16_t>::min());
        auto hi = _mm_set1_pd(std::numeric_limits<std::int16_t>::max());
        for (i = 0; i != count && i + 7 < count; i += 8)
        {
            auto quad0 = _mm256_load_pd((double*)&addr[i].mm);
            auto quad2 = _mm256_load_pd((double*)&addr[i+2].mm);
            auto quad4 = _mm256_load_pd((double*)&addr[i+4].mm);
            auto quad6 = _mm256_load_pd((double*)&addr[i+6].mm);

            quad0 = _mm256_mul_pd(quad0, quad0);
            quad2 = _mm256_mul_pd(quad2, quad2);
            quad4 = _mm256_mul_pd(quad4, quad4);
            quad6 = _mm256_mul_pd(quad6, quad6);

            quad0 = _mm256_hadd_pd(quad0, quad2);
            quad4 = _mm256_hadd_pd(quad4, quad6);

            quad0 = _mm256_sqrt_pd(quad0);
            quad4 = _mm256_sqrt_pd(quad4);

            quad0 = _mm256_mul_pd(quad0, mmscale);
            quad4 = _mm256_mul_pd(quad4, mmscale);

            // Clamp
            quad0 = _mm256_max_pd(quad0, lo);
            quad4 = _mm256_max_pd(quad4, lo);

            quad0 = _mm256_min_pd(pair0, hi);
            quad4 = _mm256_min_pd(pair4, hi);

            auto iquad0 = _mm256_cvttpd_epi32(pair0);
            auto iquad4 = _mm256_cvttpd_epi32(pair4);

            iquad4 = _mm256_shuffle_i32x4_pd(iquad4, _MM_SHUFFLE(1, 0, 1, 1));

            iquad0 = _mm_or_si128(iquad0, iquad2);
            iquad4 = _mm_or_si128(iquad4, iquad6);
            iquad0 = _mm_or_si128(iquad0, iquad4);

            _mm256_storeu_si256((__m256i*)(outputResult + i), iquad0);
        }
        for ( ; i < count; ++i) {
            auto pair0 = addr[i].mm;
            pair0 = _mm_dp_pd(pair0, pair0, 0x31);
            pair0 = _mm_sqrt_sd(_mm_setzero_pd(), pair0);
            pair0 = _mm_mul_sd(pair0, mmscale);
            pair0 = _mm_max_sd(pair0, lo);
            pair0 = _mm_min_sd(pair0, hi);
            outputResult[i] = std::int16_t(_mm_cvtsd_si32(pair0));
        }
    }
#else
    static void mod(short *outputResult, Complex const *addr, size_t count, Real scale)
    {
        size_t i;
        auto mmscale = _mm_set1_pd(scale);
        auto lo = _mm_set1_pd(std::numeric_limits<std::int16_t>::min());
        auto hi = _mm_set1_pd(std::numeric_limits<std::int16_t>::max());
        for (i = 0; i != count && i + 7 < count; i += 8)
        {
            auto pair0 = addr[i].mm;
            auto pair1 = addr[i + 1].mm;
            auto pair2 = addr[i + 2].mm;
            auto pair3 = addr[i + 3].mm;
            auto pair4 = addr[i + 4].mm;
            auto pair5 = addr[i + 5].mm;
            auto pair6 = addr[i + 6].mm;
            auto pair7 = addr[i + 7].mm;

            pair0 = _mm_mul_pd(pair0, pair0);
            pair1 = _mm_mul_pd(pair1, pair1);
            pair2 = _mm_mul_pd(pair2, pair2);
            pair3 = _mm_mul_pd(pair3, pair3);
            pair4 = _mm_mul_pd(pair4, pair4);
            pair5 = _mm_mul_pd(pair5, pair5);
            pair6 = _mm_mul_pd(pair6, pair6);
            pair7 = _mm_mul_pd(pair7, pair7);

            pair0 = _mm_hadd_pd(pair0, pair1);
            pair2 = _mm_hadd_pd(pair2, pair3);
            pair4 = _mm_hadd_pd(pair4, pair5);
            pair6 = _mm_hadd_pd(pair6, pair7);

            pair0 = _mm_sqrt_pd(pair0);
            pair2 = _mm_sqrt_pd(pair2);
            pair4 = _mm_sqrt_pd(pair4);
            pair6 = _mm_sqrt_pd(pair6);

            pair0 = _mm_mul_pd(pair0, mmscale);
            pair2 = _mm_mul_pd(pair2, mmscale);
            pair4 = _mm_mul_pd(pair4, mmscale);
            pair6 = _mm_mul_pd(pair6, mmscale);

            // Clamp
            pair0 = _mm_max_pd(pair0, lo);
            pair2 = _mm_max_pd(pair2, lo);
            pair4 = _mm_max_pd(pair4, lo);
            pair6 = _mm_max_pd(pair6, lo);

            pair0 = _mm_min_pd(pair0, hi);
            pair2 = _mm_min_pd(pair2, hi);
            pair4 = _mm_min_pd(pair4, hi);
            pair6 = _mm_min_pd(pair6, hi);

            auto ipair0 = _mm_cvttpd_epi32(pair0);
            auto ipair2 = _mm_cvttpd_epi32(pair2);
            auto ipair4 = _mm_cvttpd_epi32(pair4);
            auto ipair6 = _mm_cvttpd_epi32(pair6);

            ipair2 = _mm_shuffle_epi32(ipair2, _MM_SHUFFLE(1, 1, 0, 1));
            ipair4 = _mm_shuffle_epi32(ipair4, _MM_SHUFFLE(1, 0, 1, 1));
            ipair6 = _mm_shuffle_epi32(ipair6, _MM_SHUFFLE(0, 1, 1, 1));

            ipair0 = _mm_or_si128(ipair0, ipair2);
            ipair4 = _mm_or_si128(ipair4, ipair6);
            ipair0 = _mm_or_si128(ipair0, ipair4);

            _mm_storeu_si128((__m128i*)(outputResult + i), ipair0);
        }
        for ( ; i < count; ++i) {
            auto pair0 = addr[i].mm;
            pair0 = _mm_dp_pd(pair0, pair0, 0x31);
            pair0 = _mm_sqrt_sd(_mm_setzero_pd(), pair0);
            pair0 = _mm_mul_sd(pair0, mmscale);
            pair0 = _mm_max_sd(pair0, lo);
            pair0 = _mm_min_sd(pair0, hi);
            outputResult[i] = std::int16_t(_mm_cvtsd_si32(pair0));
        }
    }
#endif

    void stream_store(Complex *address)
    {
        _mm_stream_pd((double*)&address->mm, mm);
    }

private:
    explicit Complex(__m128d mm) : mm(mm) {}

    __m128d mm;
};

#if 0
template<>
class __attribute__((aligned(16))) Complex<float>
{
public:
    typedef float Real;

    Complex() noexcept
    {
        mm = _mm_setzero_ps();
    }

    Complex(Real re) noexcept
    {
        mm = _mm_set_ps(0.0f, re, 0.0f, 0.0f);
    }

    Complex(Real re, Real im) noexcept
    {
        mm = _mm_set_ps(0.0f, re, 0.0f, im);
    }

    Real real() const noexcept
    {
        return _mm_cvtss_f32(_mm_shuffle_ps(mm, mm, _MM_SHUFFLE(3, 3, 3, 2)));
    }

    Real imag() const noexcept
    {
        return _mm_cvtss_f32(mm);
    }

    Complex operator +(Complex const& c) const noexcept
    {
        return Complex(_mm_add_ps(mm, c.mm));
    }

    Complex& operator +=(Complex const& c) noexcept
    {
        mm = _mm_add_ps(mm, c.mm);
        return *this;
    }

    Complex operator -(Complex const& c) const noexcept
    {
        return Complex(_mm_sub_ps(mm, c.mm));
    }

    Complex& operator -=(Complex const& c) noexcept
    {
        mm = _mm_sub_ps(mm, c.mm);
        return *this;
    }

    Complex operator *(Complex const& c) const noexcept
    {
        auto imagTmp = _mm_shuffle_ps(mm, mm, _MM_SHUFFLE(3, 0, 3, 2));
        imagTmp = _mm_dp_ps(imagTmp, c.mm, 0x51);
        // Flip sign of the imaginary part
        auto t = _mm_xor_ps(mm, _mm_set_ps(0.0f, 0.0f, 0.0f, -0.0f));
        auto realTmp = _mm_dp_ps(c.mm, t, 0x51);
        auto result = _mm_shuffle_ps(imagTmp, realTmp, _MM_SHUFFLE(3, 0, 3, 0));
        return Complex(result);
    }

    Complex& operator *=(Complex const& c) noexcept
    {
        auto imagTmp = _mm_shuffle_ps(mm, mm, _MM_SHUFFLE(3, 0, 3, 2));
        imagTmp = _mm_dp_ps(imagTmp, c.mm, 0x51);
        // Flip sign of the imaginary part
        auto t = _mm_xor_ps(mm, _mm_set_ps(0.0f, 0.0f, 0.0f, -0.0f));
        auto realTmp = _mm_dp_ps(c.mm, t, 0x51);
        mm = _mm_shuffle_ps(imagTmp, realTmp, _MM_SHUFFLE(3, 0, 3, 0));
        return *this;
    }

    Complex operator -()
    {
        return Complex(_mm_xor_ps(mm, _mm_set_ps(0.0f, -0.0f, 0.0f, -0.0f)));
    }

    Real mod() const
    {
        auto tmp = _mm_dp_ps(mm, mm, 0x51);
        return _mm_cvtss_f32(_mm_sqrt_ss(tmp));
    }

    static void mod(Real *outputResult, Complex const *addr, size_t count, Real scale)
    {
        size_t i;
        auto mmscale = _mm_set1_ps(scale);
        for (i = 0; i + 3 < count; i += 4)
        {
            auto pair0 = addr[i].mm;
            auto pair1 = addr[i + 1].mm;
            auto pair2 = addr[i + 2].mm;
            auto pair3 = addr[i + 3].mm;

            pair0 = _mm_dp_ps(pair0, pair0, 0x51);
            pair1 = _mm_dp_ps(pair1, pair1, 0x52);
            pair0 = _mm_or_ps(pair0, pair1);
            pair2 = _mm_dp_ps(pair2, pair2, 0x54);
            pair3 = _mm_dp_ps(pair3, pair3, 0x58);
            pair2 = _mm_or_ps(pair2, pair3);
            pair0 = _mm_or_ps(pair0, pair2);

            pair0 = _mm_sqrt_ps(pair0);
            pair0 = _mm_mul_ps(pair0, mmscale);

            _mm_store_ps(outputResult + i, pair0);
        }
        for ( ; i < count; ++i)
            _mm_store_ss(outputResult + i,
                    _mm_mul_ss(_mm_sqrt_ss(
                    _mm_dp_ps(addr[i].mm, addr[i].mm, 0x51)),
                    mmscale));
    }

    void stream_store(Complex *address)
    {
        _mm_stream_ps((float*)&address->mm, mm);
    }

private:
    explicit Complex(__m128 mm) : mm(mm) {}

    __m128 mm;
};
#endif
#endif


template<typename T>
static inline T ComplexMagnitude(Complex<T> const& n) { return n.mod(); }

#else
template<typename T>
using Complex = std::complex<T>;

template<typename T>
static inline T ComplexMagnitude(Complex<T> const& n) { return std::abs(n); }
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

    static constexpr std::size_t log2Points = logPoints;
    static constexpr std::size_t points = 1 << logPoints;
    static constexpr std::size_t halfPoints = 1 << (logPoints-1);

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
            volatile Real tmp = (x[point].mod() * normalizer +
                                 Quantizer<Real>::value);
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
                auto const& u = w[level][j];
                for (std::size_t i = j; i < points; i += increm)
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
    }

    void process()
    {
        transform();
        Complex<Real>::mod(output.data(),
                           x.data(),
                           halfPoints,
                           normalizer);
    }

    // Outputs halfPoints result values
    //template<typename OutputIt,
    //         typename OV = typename std::iterator_traits<OutputIt>::value_type,
    //         typename = typename std::is_convertible<Real, OV>::type>
    //void process(double* out, Real scale = Real(1)) noexcept
    //{
    //    transform();
    //    Complex<Real>::mod(out,
    //                       x.data(),
    //                       halfPoints,
    //                       scale);
    //}
    //
    //// Outputs halfPoints result values
    //template<typename OutputIt,
    //         typename OV = typename std::iterator_traits<OutputIt>::value_type,
    //         typename = typename std::is_convertible<Real, OV>::type>
    //void process(short* out, Real scale = Real(1)) noexcept
    //{
    //    transform();
    //    Complex<Real>::mod(out,
    //                       x.data(),
    //                       halfPoints,
    //                       scale);
    //}

    // Outputs halfPoints result values
    template<typename OutputIt,
             typename OV = typename std::iterator_traits<OutputIt>::value_type,
             typename = typename std::is_convertible<Real, OV>::type>
    void process(OutputIt out, Real scale = Real(1)) noexcept
    {
        transform();
        Complex<Real>::mod(out,
                           x.data(),
                           halfPoints,
                           scale);
    }

    template<typename InputIt, typename U = Real,
             typename IV = typename std::iterator_traits<InputIt>::value_type,
             typename = typename std::enable_if<std::is_convertible<IV, Real>::value>::type,
             typename OutputIt,
             typename OV = typename std::iterator_traits<OutputIt>::value_type>
    void process(InputIt st, InputIt en, OutputIt out, U scale = Real(1)) noexcept
    {
        copyIn(st, en);
        process(out, scale);
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
        std::size_t n = std::distance(st, en);

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
        for (std::size_t i = 0, e = points;
             i != e; ++i)
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
            for (std::size_t i = 0; i < points; ++i)
            {
                Real re = std::cos(Real(2) * Real(3.14159265358979323) * i / l2);
                Real im = -std::sin(Real(2) * Real(3.14159265358979323) * i / l2);
                w[level][i] = Complex<Real>(re, im);
            }
            l2 *= 2;
        }

        // Generate bit reversed index lookup table
        int rev = 0;
        for (std::size_t i = 0; i < points - 1; ++i)
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
    static constexpr Real normalizer = normalizerLookup[logPoints];

    // No dynamic allocation!
    // All memory buffers are fixed size and part of this object

    typedef std::array<Real, points> TapeContainer;
    typedef std::array<int, points> BitRevContainer;
    typedef std::array<Complex<Real>, points> XContainer;
    typedef std::array<XContainer, logPoints> WContainer;

    // Gaps are there to peturb the offsets and avoid set conflicts

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


