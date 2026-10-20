#pragma once
#include <vector>
#include <cmath>

// YIN pitch detector for the tuner (runs on the UI thread on a copy of the input).
struct Tuner
{
    static float detect (const std::vector<float>& x, double sr)
    {
        const int N = (int) x.size(), W = N / 2;
        const int minTau = (int) (sr / 1400.0), maxTau = std::min (W - 2, (int) (sr / 38.0));
        double energy = 0;
        for (int i = 0; i < N; ++i) energy += x[(size_t) i] * x[(size_t) i];
        if (energy / N < 1e-7 || maxTau <= minTau + 2) return 0.f;

        std::vector<float> d ((size_t) maxTau + 1, 0.f);
        for (int tau = 1; tau <= maxTau; ++tau)
        {
            double s = 0;
            for (int i = 0; i < W; ++i) { const double v = x[(size_t) i] - x[(size_t) (i + tau)]; s += v * v; }
            d[(size_t) tau] = (float) s;
        }
        // cumulative mean normalised difference
        double run = 0;
        d[0] = 1.f;
        for (int tau = 1; tau <= maxTau; ++tau) { run += d[(size_t) tau]; d[(size_t) tau] = run > 0 ? (float) (d[(size_t) tau] * tau / run) : 1.f; }

        int best = -1;
        for (int tau = minTau; tau < maxTau; ++tau)
            if (d[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < maxTau && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau;
                best = tau; break;
            }
        if (best < 0) return 0.f;
        const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
        const float den = a - 2.f * b + c;
        const float shift = std::abs (den) > 1e-9f ? 0.5f * (a - c) / den : 0.f;
        return (float) (sr / ((double) best + shift));
    }
};
