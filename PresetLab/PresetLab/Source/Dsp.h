#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdint>

// Small DSP building blocks shared by the instruments and the effects.
namespace dsp1
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kTwoPi = 2.0 * kPi;

    inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
    inline float clampf (float v, float lo, float hi) { return std::min (hi, std::max (lo, v)); }
    inline float lerp (float a, float b, float t) { return a + (b - a) * t; }

    // decay multiplier per sample so the level falls 60 dB in t60 seconds
    inline float decayCoef (double t60, double sr) { return t60 <= 0.0 ? 0.0f : (float) std::exp (-6.907755 / (t60 * sr)); }

    struct Rng
    {
        uint32_t s = 0x12345678u;
        void seed (uint32_t v) { s = v ? v : 1u; }
        float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s & 0xffffff) / 8388608.0f - 1.0f; }  // -1..1
        float uni() { return next() * 0.5f + 0.5f; }
    };

    // RBJ biquad, transposed direct form II
    struct Biquad
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        void reset() { z1 = z2 = 0; }
        inline float process (float x)
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return (float) y;
        }
        void set (double B0, double B1, double B2, double A0, double A1, double A2)
        {
            b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
        }
        static double w (double f, double sr) { return kTwoPi * std::min (f, sr * 0.49) / sr; }
        void lowpass (double f, double q, double sr)  { const double c = std::cos (w (f, sr)), al = std::sin (w (f, sr)) / (2 * q); set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al); }
        void highpass (double f, double q, double sr) { const double c = std::cos (w (f, sr)), al = std::sin (w (f, sr)) / (2 * q); set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al); }
        void bandpass (double f, double q, double sr) { const double c = std::cos (w (f, sr)), al = std::sin (w (f, sr)) / (2 * q); set (al, 0, -al, 1 + al, -2 * c, 1 - al); }
        void peak (double f, double q, double db, double sr)
        {
            const double A = std::pow (10.0, db / 40.0), c = std::cos (w (f, sr)), al = std::sin (w (f, sr)) / (2 * q);
            set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
        }
        void lowShelf (double f, double db, double sr, double S = 1.0)
        {
            const double A = std::pow (10.0, db / 40.0), ww = w (f, sr), c = std::cos (ww), s = std::sin (ww);
            const double al = s / 2 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2), sq = 2 * std::sqrt (A) * al;
            set (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
                 (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
        }
        void highShelf (double f, double db, double sr, double S = 1.0)
        {
            const double A = std::pow (10.0, db / 40.0), ww = w (f, sr), c = std::cos (ww), s = std::sin (ww);
            const double al = s / 2 * std::sqrt ((A + 1 / A) * (1 / S - 1) + 2), sq = 2 * std::sqrt (A) * al;
            set (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
                 (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
        }
    };

    struct OnePole
    {
        float a = 0, z = 0;
        void setLowpass (double f, double sr) { a = (float) std::exp (-kTwoPi * std::min (f, sr * 0.49) / sr); }
        inline float lp (float x) { z = x + a * (z - x); return z; }
        inline float hp (float x) { return x - lp (x); }
        void reset() { z = 0; }
    };

    // Zavalishin TPT state-variable filter (stable under fast modulation)
    struct Svf
    {
        float ic1 = 0, ic2 = 0, g = 0, k = 1, a1 = 0, a2 = 0, a3 = 0;
        void reset() { ic1 = ic2 = 0; }
        void set (double f, double res01, double sr)   // res01 0..1
        {
            g = (float) std::tan (kPi * std::min (f, sr * 0.47) / sr);
            k = (float) (2.0 - 1.96 * std::min (0.999, std::max (0.0, res01)));
            a1 = 1.0f / (1.0f + g * (g + k)); a2 = g * a1; a3 = g * a2;
        }
        struct Out { float lp, bp, hp; };
        inline Out tick (float x)
        {
            const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2 * v1 - ic1; ic2 = 2 * v2 - ic2;
            return { v2, v1, x - k * v1 - v2 };
        }
    };

    // circular delay line with linear / cubic fractional reads
    struct Delay
    {
        std::vector<float> buf; int mask = 0, pos = 0;
        void init (int maxSamples)
        {
            int n = 1; while (n < maxSamples + 4) n <<= 1;
            buf.assign ((size_t) n, 0.f); mask = n - 1; pos = 0;
        }
        void clear() { std::fill (buf.begin(), buf.end(), 0.f); }
        inline void push (float x) { buf[(size_t) pos] = x; pos = (pos + 1) & mask; }
        inline float at (int d) const { return buf[(size_t) ((pos - 1 - d) & mask)]; }
        inline float read (float d) const   // d samples behind the newest sample (>= 0)
        {
            const int i = (int) d; const float f = d - (float) i;
            const float x0 = at (i - 1 < 0 ? 0 : i - 1), x1 = at (i), x2 = at (i + 1), x3 = at (i + 2);
            const float c1 = 0.5f * (x2 - x0), c2 = x0 - 2.5f * x1 + 2.f * x2 - 0.5f * x3, c3 = 0.5f * (x3 - x0) + 1.5f * (x1 - x2);
            return ((c3 * f + c2) * f + c1) * f + x1;
        }
    };

    // band-limited step correction for the virtual analog oscillators
    inline float polyBlep (float t, float dt)
    {
        if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
        if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
        return 0.f;
    }

    inline float softClip (float x) { return x / (1.f + std::abs (x)); }
    inline float fastTanh (float x)
    {
        x = clampf (x, -4.5f, 4.5f);
        const float x2 = x * x;
        return x * (27.f + x2) / (27.f + 9.f * x2);
    }
}
