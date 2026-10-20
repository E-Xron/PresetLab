#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
#include "Params.h"

using namespace dsp1;

// ------------------------------------------------------------------------------------------------
// One stomp or rack slot. It can be any of the effect types; switching type clears its state.
class FxUnit
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        dryL.assign ((size_t) maxBlock, 0.f); dryR.assign ((size_t) maxBlock, 0.f);
        for (auto& d : dl) d.init ((int) (sr * 2.2));
        reverb.setSampleRate (sr);
        timeSm.reset (sr, 0.25);
        lastType = -1;
    }

    void process (float* L, float* R, int n, int type, const float* k, float mix)
    {
        if (type != lastType) { reset(); lastType = type; }
        if ((size_t) n > dryL.size()) { dryL.resize ((size_t) n); dryR.resize ((size_t) n); }
        std::copy (L, L + n, dryL.data()); std::copy (R, R + n, dryR.data());

        switch (type)
        {
            case pl::Overdrive:  drive (L, R, n, k, 0); break;
            case pl::Distortion: drive (L, R, n, k, 1); break;
            case pl::Fuzz:       drive (L, R, n, k, 2); break;
            case pl::Compressor: compressor (L, R, n, k); break;
            case pl::NoiseGate:  gate (L, R, n, k); break;
            case pl::Chorus:     modDelay (L, R, n, k, 0); break;
            case pl::Flanger:    modDelay (L, R, n, k, 1); break;
            case pl::Phaser:     phaser (L, R, n, k); break;
            case pl::Tremolo:    tremolo (L, R, n, k); break;
            case pl::AutoWah:    autowah (L, R, n, k); break;
            case pl::Eq3:        eq3 (L, R, n, k); break;
            case pl::Delay:      delay (L, R, n, k, 0); break;
            case pl::PingPong:   delay (L, R, n, k, 1); break;
            case pl::TapeEcho:   delay (L, R, n, k, 2); break;
            case pl::RoomVerb: case pl::HallVerb: case pl::PlateVerb: case pl::SpringVerb: verb (L, R, n, k, type); break;
            case pl::Rotary:     rotary (L, R, n, k); break;
            case pl::AutoFilter: autofilter (L, R, n, k); break;
            case pl::BitCrusher: crusher (L, R, n, k); break;
            case pl::Widener:    widener (L, R, n, k); break;
            case pl::LoFi:       lofi (L, R, n, k); break;
            default: break;
        }

        mixSm.setTargetValue (mix);
        for (int i = 0; i < n; ++i)
        {
            const float m = mixSm.getNextValue();
            L[i] = dryL[(size_t) i] + (L[i] - dryL[(size_t) i]) * m;
            R[i] = dryR[(size_t) i] + (R[i] - dryR[(size_t) i]) * m;
        }
    }

    // gain reduction / activity readout for the UI (0..1)
    std::atomic<float> meter { 0.f };

private:
    void reset()
    {
        for (auto& d : dl) d.clear();
        for (auto& b : bq) b.reset();
        for (auto& o : op) o.reset();
        for (auto& s : sv) s.reset();
        for (auto& a : ap) a = 0.f;
        reverb.reset();
        env = 0.f; gGain = 1.f; lfo = lfo2 = 0.0; hornSpeed = drumSpeed = -1.f; fbL = fbR = 0.f;
        holdL = holdR = 0.f; holdCount = 0; crackle = 0.f; lastEq[0] = -1.f; envC[0] = envC[1] = 0.f;
        mixSm.reset (sr, 0.03); mixSm.setCurrentAndTargetValue (0.f); timeSm.setCurrentAndTargetValue (0.f);
    }

    static float lvl (float k) { return dbToGain ((k - 0.5f) * 24.f); }

    // 0 overdrive, 1 distortion, 2 fuzz
    void drive (float* L, float* R, int n, const float* k, int kind)
    {
        const float g = kind == 0 ? 1.f + 40.f * k[0] * k[0] : kind == 1 ? 2.f + 220.f * k[0] * k[0] : 6.f + 500.f * k[0] * k[0];
        const float out = lvl (k[2]) * (kind == 0 ? 0.5f : 0.35f);
        bq[0].highpass (kind == 0 ? 600.0 : kind == 1 ? 120.0 : 80.0, 0.7, sr);  bq[1] = bq[0];          // pre filter (L, R)
        bq[2].lowpass (kind == 2 ? 600.0 + k[1] * 400.0 : 900.0 + k[1] * k[1] * 9000.0, 0.7, sr); bq[3] = bq[2]; // tone
        if (kind == 2) { bq[4].highpass (800.0 + k[1] * 400.0, 0.7, sr); bq[5] = bq[4]; }
        op[0].setLowpass (700.0, sr); op[1].setLowpass (700.0, sr);
        const float gateThr = kind == 2 ? dbToGain (-80.f + k[3] * 45.f) : 0.f;
        float* ch[2] { L, R };
        for (int c = 0; c < 2; ++c)
        {
            auto& pre = bq[(size_t) c]; auto& tone = bq[(size_t) (2 + c)]; auto& tone2 = bq[(size_t) (4 + c)];
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                float y;
                if (kind == 0)
                {
                    const float h = pre.process (x);
                    y = fastTanh (h * g + 0.1f) - fastTanh (0.1f) + op[(size_t) c].lp (x) * 0.6f;      // mid-hump drive, clean lows kept
                }
                else if (kind == 1)
                {
                    const float h = pre.process (x) * g;
                    y = softClip (clampf (h, -1.5f, 1.5f) * 1.2f) * 1.4f;
                }
                else
                {
                    const float h = pre.process (x) * g;
                    y = h > 0 ? fastTanh (h) : fastTanh (h * 0.6f) * 0.9f;
                    y = clampf (y * 1.6f, -1.f, 1.f);
                    const float a = std::abs (x); envC[c] = std::max (a, envC[c] * 0.9995f);
                    if (envC[c] < gateThr) y *= envC[c] / std::max (1e-9f, gateThr);
                }
                if (kind == 2) { const float lo = tone.process (y), hi = tone2.process (y); y = lo * (1.f - k[1]) + hi * k[1]; }
                else y = tone.process (y);
                ch[c][i] = y * out;
            }
        }
        meter = clampf (k[0], 0.f, 1.f);
    }

    void compressor (float* L, float* R, int n, const float* k)
    {
        const float thr = -8.f - 40.f * k[0], ratio = 3.f + 7.f * k[0];
        const float att = std::exp (-1.f / (float) (sr * (0.0005 + 0.05 * k[1] * k[1]))), rel = std::exp (-1.f / (float) (sr * 0.18));
        const float makeup = dbToGain (-thr * (1.f - 1.f / ratio) * 0.55f) * lvl (k[2]);
        float maxGr = 0.f;
        for (int i = 0; i < n; ++i)
        {
            const float a = std::max (std::abs (L[i]), std::abs (R[i]));
            env = a > env ? a + att * (env - a) : a + rel * (env - a);
            const float db = juce::Decibels::gainToDecibels (env + 1e-9f);
            const float gr = db > thr ? (db - thr) * (1.f - 1.f / ratio) : 0.f;
            const float g = dbToGain (-gr) * makeup;
            L[i] *= g; R[i] *= g;
            maxGr = std::max (maxGr, gr);
        }
        meter = clampf (maxGr / 20.f, 0.f, 1.f);
    }

    void gate (float* L, float* R, int n, const float* k)
    {
        const float thr = dbToGain (-80.f + 60.f * k[0]);
        const float rel = std::exp (-1.f / (float) (sr * (0.01 + 0.5 * k[1]))), atk = std::exp (-1.f / (float) (sr * 0.001));
        for (int i = 0; i < n; ++i)
        {
            const float a = std::max (std::abs (L[i]), std::abs (R[i]));
            env = std::max (a, env * 0.9997f);
            const float target = env > thr ? 1.f : 0.f;
            gGain = target > gGain ? target + atk * (gGain - target) : target + rel * (gGain - target);
            L[i] *= gGain; R[i] *= gGain;
        }
        meter = 1.f - gGain;
    }

    // 0 chorus, 1 flanger
    void modDelay (float* L, float* R, int n, const float* k, int kind)
    {
        const double rate = kind == 0 ? 0.08 + 4.0 * k[0] * k[0] : 0.03 + 2.5 * k[0] * k[0];
        const float base = kind == 0 ? 0.012f : 0.0003f + 0.006f * k[3];
        const float depth = kind == 0 ? 0.0065f * k[1] : (base * 0.95f) * k[1];
        const float fb = kind == 1 ? (k[2] - 0.5f) * 1.8f : 0.f;
        op[0].setLowpass (2000.0 + k[2] * k[2] * 14000.0, sr); op[1].a = op[0].a;
        for (int i = 0; i < n; ++i)
        {
            lfo += rate / sr; if (lfo >= 1.0) lfo -= 1.0;
            const float s1 = std::sin ((float) (kTwoPi * lfo)), s2 = std::sin ((float) (kTwoPi * (lfo + 0.25)));
            const float dL = (float) ((base + depth * (0.5f + 0.5f * s1)) * sr), dR = (float) ((base + depth * (0.5f + 0.5f * s2)) * sr);
            float wl = dl[0].read (dL), wr = dl[1].read (dR);
            dl[0].push (L[i] + wl * fb); dl[1].push (R[i] + wr * fb);
            if (kind == 0) { wl = op[0].lp (wl); wr = op[1].lp (wr); }
            L[i] = kind == 0 ? wl : (L[i] + wl) * 0.6f;
            R[i] = kind == 0 ? wr : (R[i] + wr) * 0.6f;
        }
    }

    void phaser (float* L, float* R, int n, const float* k)
    {
        const double rate = 0.04 + 4.0 * k[0] * k[0];
        const float fb = k[2] * 0.85f;
        for (int i = 0; i < n; ++i)
        {
            lfo += rate / sr; if (lfo >= 1.0) lfo -= 1.0;
            for (int c = 0; c < 2; ++c)
            {
                const float s = 0.5f + 0.5f * std::sin ((float) (kTwoPi * (lfo + c * 0.25)));
                const float f = 200.f * std::exp2 (s * 4.3f * k[1]);
                const float tn = std::tan ((float) kPi * std::min (f, (float) sr * 0.45f) / (float) sr);
                const float a = (tn - 1.f) / (tn + 1.f);
                float x = (c == 0 ? L[i] : R[i]) + (c == 0 ? fbL : fbR) * fb;
                for (int st = 0; st < 6; ++st)
                {
                    float& z = ap[c * 6 + st];
                    const float y = a * x + z; z = x - a * y; x = y;
                }
                if (c == 0) { fbL = x; L[i] = (L[i] + x) * 0.6f; } else { fbR = x; R[i] = (R[i] + x) * 0.6f; }
            }
        }
    }

    void tremolo (float* L, float* R, int n, const float* k)
    {
        const double rate = 1.0 + 11.0 * k[0];
        const float sharp = 1.f + 20.f * k[2] * k[2];
        for (int i = 0; i < n; ++i)
        {
            lfo += rate / sr; if (lfo >= 1.0) lfo -= 1.0;
            float s = fastTanh (std::sin ((float) (kTwoPi * lfo)) * sharp) / fastTanh (sharp);
            const float g = 1.f - k[1] * (0.5f + 0.5f * s);
            L[i] *= g; R[i] *= g;
        }
    }

    void autowah (float* L, float* R, int n, const float* k)
    {
        const float atk = std::exp (-1.f / (float) (sr * 0.004)), rel = std::exp (-1.f / (float) (sr * 0.12));
        for (int i = 0; i < n; i += 16)
        {
            const int m = std::min (16, n - i);
            for (int j = 0; j < m; ++j)
            {
                const float a = std::abs (L[i + j] + R[i + j]) * 0.5f;
                env = a > env ? a + atk * (env - a) : a + rel * (env - a);
            }
            const float e = clampf (env * (2.f + 30.f * k[0] * k[0]), 0.f, 1.f);
            const double f = 320.0 * std::exp2 (e * (1.0 + 4.0 * k[1]));
            sv[0].set (f, 0.55 + 0.42 * k[2], sr); sv[1].set (f, 0.55 + 0.42 * k[2], sr);
            for (int j = 0; j < m; ++j)
            {
                L[i + j] = sv[0].tick (L[i + j]).bp * 2.2f + L[i + j] * 0.15f;
                R[i + j] = sv[1].tick (R[i + j]).bp * 2.2f + R[i + j] * 0.15f;
            }
        }
        meter = clampf (env * 4.f, 0.f, 1.f);
    }

    void eq3 (float* L, float* R, int n, const float* k)
    {
        if (k[0] != lastEq[0] || k[1] != lastEq[1] || k[2] != lastEq[2] || k[3] != lastEq[3])
        {
            bq[0].lowShelf (120.0, (k[0] - 0.5) * 30.0, sr);
            bq[2].peak (250.0 * std::pow (20.0, (double) k[3]), 0.9, (k[1] - 0.5) * 30.0, sr);
            bq[4].highShelf (5000.0, (k[2] - 0.5) * 30.0, sr);
            for (int i = 0; i < 6; i += 2) { auto z1 = bq[(size_t) i + 1].z1, z2 = bq[(size_t) i + 1].z2; bq[(size_t) i + 1] = bq[(size_t) i]; bq[(size_t) i + 1].z1 = z1; bq[(size_t) i + 1].z2 = z2; }
            for (int i = 0; i < 4; ++i) lastEq[i] = k[i];
        }
        for (int i = 0; i < n; ++i)
        {
            L[i] = bq[4].process (bq[2].process (bq[0].process (L[i])));
            R[i] = bq[5].process (bq[3].process (bq[1].process (R[i])));
        }
    }

    // 0 stereo delay, 1 ping-pong, 2 tape echo
    void delay (float* L, float* R, int n, const float* k, int kind)
    {
        const float timeS = kind == 2 ? 0.06f * std::pow (15.f, k[0]) : 0.03f * std::pow (50.f, k[0]);
        if (timeSm.getCurrentValue() <= 0.f) timeSm.setCurrentAndTargetValue (timeS * (float) sr);
        timeSm.setTargetValue (timeS * (float) sr);
        const float fb = kind == 2 ? k[1] * 1.05f : k[1] * 0.95f;
        const float toneK = kind == 2 ? k[3] : k[2];
        op[0].setLowpass (1000.0 + toneK * toneK * 15000.0, sr); op[1].a = op[0].a;
        op[2].setLowpass (kind == 2 ? 90.0 : 30.0, sr); op[3].a = op[2].a;
        const float wob = kind == 2 ? k[2] : 0.f;
        for (int i = 0; i < n; ++i)
        {
            float d = timeSm.getNextValue();
            if (wob > 0.f)
            {
                lfo += 0.5 / sr; if (lfo >= 1.0) lfo -= 1.0;
                lfo2 += 6.3 / sr; if (lfo2 >= 1.0) lfo2 -= 1.0;
                d *= 1.f + wob * (0.006f * std::sin ((float) (kTwoPi * lfo)) + 0.0015f * std::sin ((float) (kTwoPi * lfo2)));
            }
            d = clampf (d, 1.f, (float) dl[0].mask - 8.f);
            float wl = dl[0].read (d), wr = dl[1].read (d);
            wl = op[0].lp (wl); wr = op[1].lp (wr);
            wl -= op[2].lp (wl); wr -= op[3].lp (wr);
            if (kind == 2) { wl = fastTanh (wl * 1.2f) / 1.2f; wr = fastTanh (wr * 1.2f) / 1.2f; }
            if (kind == 1)
            {
                dl[0].push ((L[i] + R[i]) * 0.5f + wr * fb);
                dl[1].push (wl * fb);
            }
            else
            {
                dl[0].push (L[i] + wl * fb);
                dl[1].push (R[i] + wr * fb);
            }
            L[i] = wl; R[i] = wr;
        }
    }

    void verb (float* L, float* R, int n, const float* k, int type)
    {
        juce::Reverb::Parameters p;
        p.wetLevel = 1.f; p.dryLevel = 0.f; p.width = 1.f; p.freezeMode = 0.f;
        float pre = 0.f;
        switch (type)
        {
            case pl::RoomVerb:  p.roomSize = 0.25f + 0.45f * k[0]; p.damping = 0.2f + 0.7f * k[1]; pre = k[2] * 0.08f; break;
            case pl::HallVerb:  p.roomSize = 0.62f + 0.37f * k[0]; p.damping = 0.1f + 0.7f * k[1]; pre = k[2] * 0.12f; break;
            case pl::PlateVerb: p.roomSize = 0.5f + 0.45f * k[0];  p.damping = 0.05f + 0.4f * k[1]; pre = k[2] * 0.08f; break;
            default:            p.roomSize = 0.35f + 0.5f * k[0];  p.damping = 0.6f - 0.5f * k[1]; pre = 0.02f; break;
        }
        reverb.setParameters (p);
        const float pd = pre * (float) sr;
        const bool spring = type == pl::SpringVerb;
        const float drip = 0.55f + 0.4f * k[2];
        if (spring) { bq[0].bandpass (600.0 + k[1] * 2500.0, 0.6, sr); bq[1] = bq[0]; }
        if (type == pl::PlateVerb) { bq[2].highpass (220.0, 0.7, sr); bq[3] = bq[2]; }
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            if (pd > 1.f) { dl[0].push (l); dl[1].push (r); l = dl[0].read (pd); r = dl[1].read (pd); }
            if (type == pl::PlateVerb) { l = bq[2].process (l); r = bq[3].process (r); }
            L[i] = l * 0.5f; R[i] = r * 0.5f;
        }
        reverb.processStereo (L, R, n);
        if (spring)
            for (int i = 0; i < n; ++i)
            {
                float x[2] { bq[0].process (L[i]) * 2.f, bq[1].process (R[i]) * 2.f };
                for (int c = 0; c < 2; ++c)
                    for (int st = 0; st < 8; ++st)
                    {
                        float& z = ap[c * 8 + st];
                        const float y = -drip * x[c] + z; z = x[c] + drip * y; x[c] = y;
                    }
                L[i] = x[0]; R[i] = x[1];
            }
    }

    void rotary (float* L, float* R, int n, const float* k)
    {
        const bool fast = k[0] > 0.5f;
        const float hornT = fast ? 6.8f : 0.8f, drumT = fast ? 5.9f : 0.65f;
        if (hornSpeed < 0.f) { hornSpeed = hornT; drumSpeed = drumT; }
        const float accH = std::exp (-1.f / (float) (sr * 0.7)), accD = std::exp (-1.f / (float) (sr * 2.5));
        const float drv = 1.f + k[1] * 6.f, drvN = 1.f / fastTanh (drv);
        bq[0].lowpass (800.0, 0.7, sr); bq[1].highpass (800.0, 0.7, sr);
        for (int i = 0; i < n; ++i)
        {
            hornSpeed = hornT + accH * (hornSpeed - hornT);
            drumSpeed = drumT + accD * (drumSpeed - drumT);
            lfo += hornSpeed / sr; if (lfo >= 1.0) lfo -= 1.0;
            lfo2 += drumSpeed / sr; if (lfo2 >= 1.0) lfo2 -= 1.0;
            const float x = fastTanh ((L[i] + R[i]) * 0.5f * drv) * drvN;
            const float lo = bq[0].process (x), hi = bq[1].process (x);
            const float hs = std::sin ((float) (kTwoPi * lfo)), hc = std::cos ((float) (kTwoPi * lfo));
            dl[0].push (hi);
            const float hd = dl[0].read ((float) (sr * (0.0012 + 0.00025 * hs)));
            const float ds = std::sin ((float) (kTwoPi * lfo2));
            const float hornL = hd * (0.75f + 0.25f * hc) * (1.f + 0.5f * hs), hornR = hd * (0.75f + 0.25f * hc) * (1.f - 0.5f * hs);
            const float drum = lo * (0.85f + 0.15f * ds);
            const float hb = 0.4f + 0.6f * k[2], db = 1.4f - 0.6f * k[2];
            L[i] = hornL * hb + drum * db * (1.f + 0.2f * ds);
            R[i] = hornR * hb + drum * db * (1.f - 0.2f * ds);
        }
    }

    void autofilter (float* L, float* R, int n, const float* k)
    {
        const double rate = 0.05 + 8.0 * k[0] * k[0];
        const double base = 100.0 * std::pow (80.0, (double) k[2]);
        for (int i = 0; i < n; i += 16)
        {
            const int m = std::min (16, n - i);
            lfo += rate * m / sr; if (lfo >= 1.0) lfo -= 1.0;
            const double f = juce::jlimit (30.0, sr * 0.45, base * std::exp2 (k[1] * 3.0 * std::sin (kTwoPi * lfo)));
            sv[0].set (f, k[3] * 0.92, sr); sv[1].set (f, k[3] * 0.92, sr);
            for (int j = 0; j < m; ++j) { L[i + j] = sv[0].tick (L[i + j]).lp; R[i + j] = sv[1].tick (R[i + j]).lp; }
        }
    }

    void crusher (float* L, float* R, int n, const float* k)
    {
        const float steps = std::exp2 (16.f - 13.f * k[0]);
        const int hold = 1 + (int) (k[1] * k[1] * 40.f);
        op[0].setLowpass (1500.0 + k[2] * k[2] * 18000.0, sr); op[1].a = op[0].a;
        for (int i = 0; i < n; ++i)
        {
            if (holdCount++ % hold == 0) { holdL = std::round (L[i] * steps) / steps; holdR = std::round (R[i] * steps) / steps; }
            L[i] = op[0].lp (holdL); R[i] = op[1].lp (holdR);
        }
    }

    void widener (float* L, float* R, int n, const float* k)
    {
        const float w = k[0] * 2.f, haas = k[1] * 0.02f * (float) sr;
        for (int i = 0; i < n; ++i)
        {
            const float m = (L[i] + R[i]) * 0.5f, s = (L[i] - R[i]) * 0.5f * w;
            float l = m + s, r = m - s;
            if (haas > 1.f) { dl[0].push (r); r = r * (1.f - k[1] * 0.5f) + dl[0].read (haas) * k[1] * 0.5f; }
            L[i] = l; R[i] = r;
        }
    }

    void lofi (float* L, float* R, int n, const float* k)
    {
        op[0].setLowpass (12000.0 * std::pow (0.2, (double) k[2]), sr); op[1].a = op[0].a;
        op[2].setLowpass (40.0 + 200.0 * k[2], sr); op[3].a = op[2].a;
        const float hiss = k[1] * 0.015f;
        for (int i = 0; i < n; ++i)
        {
            lfo += 0.55 / sr; if (lfo >= 1.0) lfo -= 1.0;
            lfo2 += 7.0 / sr; if (lfo2 >= 1.0) lfo2 -= 1.0;
            const float d = (float) (sr * (0.004 + k[0] * (0.0025 * std::sin (kTwoPi * lfo) + 0.0003 * std::sin (kTwoPi * lfo2))));
            dl[0].push (L[i]); dl[1].push (R[i]);
            float l = dl[0].read (d), r = dl[1].read (d);
            l = fastTanh (l * (1.f + k[2])) / (1.f + k[2] * 0.5f); r = fastTanh (r * (1.f + k[2])) / (1.f + k[2] * 0.5f);
            l = op[0].lp (l); r = op[1].lp (r);
            l -= op[2].lp (l); r -= op[3].lp (r);
            if (rng.uni() < k[1] * 0.0004f) crackle = rng.next() * k[1] * 0.3f;
            const float nz = rng.next() * hiss + crackle; crackle *= 0.7f;
            L[i] = l + nz; R[i] = r + nz;
        }
    }

    double sr = 48000.0;
    int lastType = -1;
    std::vector<float> dryL, dryR;
    Delay dl[2];
    Biquad bq[6];
    OnePole op[4];
    Svf sv[2];
    float ap[16] {};
    float env = 0.f, envC[2] {}, gGain = 1.f, fbL = 0.f, fbR = 0.f, hornSpeed = -1.f, drumSpeed = -1.f;
    float holdL = 0.f, holdR = 0.f, crackle = 0.f, lastEq[4] { -1.f, -1.f, -1.f, -1.f };
    int holdCount = 0;
    double lfo = 0.0, lfo2 = 0.0;
    Rng rng;
    juce::Reverb reverb;
    juce::LinearSmoothedValue<float> timeSm { 0.f }, mixSm { 1.f };
};

// ------------------------------------------------------------------------------------------------
// Amp head: input filter, 1-3 tube-like gain stages (2x oversampled), tone stack, power amp.
class AmpSim
{
public:
    struct Model { float hp, preMin, preMax; int stages; float asym, midF, midDb, brightDb, postLp, outDb, lowBlend; };
    static const Model& model (int m)
    {
        static const Model list[] {
            { 60.f,  0.f, 26.f, 1, 0.10f,  500.f, -4.f,  3.f,  9000.f,  0.f, 0.f },   // Clean American
            { 50.f, -6.f, 20.f, 1, 0.05f,  700.f, -1.f, -2.f,  6000.f,  1.f, 0.f },   // Jazz Clean
            { 90.f,  8.f, 38.f, 2, 0.20f,  900.f,  3.f,  2.f,  7000.f, -2.f, 0.f },   // British Crunch
            { 110.f, 18.f, 50.f, 3, 0.25f,  750.f,  2.f,  1.f,  6500.f, -4.f, 0.f },  // Hot Rod Lead
            { 140.f, 20.f, 54.f, 3, 0.15f,  650.f, -5.f,  3.f,  7000.f, -4.f, 0.f },  // Modern Metal
            { 30.f,  0.f, 30.f, 2, 0.15f,  500.f,  0.f,  0.f,  5000.f, -1.f, 0.5f },  // Bass Tube
            { 35.f, -3.f, 28.f, 1, 0.10f,  800.f,  1.f,  2.f,  8000.f,  0.f, 0.4f },  // Bass Modern
            { 25.f, -6.f, 20.f, 1, 0.05f, 1000.f,  0.f,  0.f, 16000.f,  0.f, 0.f },   // Keys Hi-Fi
            { 200.f, 6.f, 36.f, 2, 0.30f, 1200.f,  5.f,  0.f,  3500.f, -2.f, 0.f },   // Lo-Fi Practice
        };
        return list[juce::jlimit (0, 8, m)];
    }

    void prepare (double sampleRate, int maxBlock)
    {
        sr = sampleRate;
        os.initProcessing ((size_t) maxBlock);
        os.reset();
        lastKey = -1.f;
    }

    void process (float* L, float* R, int n, int m, const float* k /* gain bass mid treble presence master */)
    {
        const auto& md = model (m);
        const float key = (float) m * 1000.f + k[1] * 100.f + k[2] * 10.f + k[3] + k[4] * 0.1f;
        if (key != lastKey)
        {
            lastKey = key;
            for (int c = 0; c < 2; ++c)
            {
                pre[c][0].highpass (md.hp, 0.7, sr);
                pre[c][1].highShelf (1500.0, md.brightDb, sr);
                post[c][0].lowShelf (120.0, (k[1] - 0.5) * 24.0, sr);
                post[c][1].peak (md.midF, 0.8, md.midDb + (k[2] - 0.5) * 20.0, sr);
                post[c][2].highShelf (2500.0, (k[3] - 0.5) * 24.0, sr);
                post[c][3].highShelf (5000.0, (k[4] - 0.5) * 16.0, sr);
                post[c][4].lowpass (md.postLp, 0.7, sr);
                low[c].lowpass (200.0, 0.7, sr);
            }
            for (int c = 0; c < 2; ++c) for (int s = 0; s < 3; ++s) inter[c][s].setLowpass (s == 0 ? 9000.0 : 6500.0, sr * 2.0);
        }
        const float preG = dbToGain (md.preMin + (md.preMax - md.preMin) * k[0]);
        const float outG = dbToGain (-22.f + 28.f * k[5] + md.outDb);
        float* ch[2] { L, R };
        float lowKeep[2][512];
        const int nn = std::min (n, 512);

        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < nn; ++i)
            {
                float x = pre[c][1].process (pre[c][0].process (ch[c][i]));
                lowKeep[c][i] = md.lowBlend > 0.f ? low[c].process (ch[c][i]) * md.lowBlend : 0.f;
                ch[c][i] = x * preG;
            }

        juce::dsp::AudioBlock<float> block (ch, 2, (size_t) nn);
        auto up = os.processSamplesUp (block);
        for (int c = 0; c < 2; ++c)
        {
            float* d = up.getChannelPointer ((size_t) c);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
            {
                float y = d[i];
                for (int s = 0; s < md.stages; ++s)
                {
                    const float b = md.asym * (s == 0 ? 1.f : 0.6f);
                    y = fastTanh (y + b) - fastTanh (b);
                    if (s + 1 < md.stages) { y = inter[c][s].lp (y) * 3.2f; dcState[c][s] = y - dcX[c][s] + 0.999f * dcState[c][s]; dcX[c][s] = y; y = dcState[c][s]; }
                }
                d[i] = y;
            }
        }
        os.processSamplesDown (block);

        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < nn; ++i)
            {
                float y = ch[c][i];
                for (int s = 0; s < 4; ++s) y = post[c][s].process (y);
                y = softClip (y * 1.5f) * 1.2f + lowKeep[c][i];
                y = post[c][4].process (y);
                ch[c][i] = y * outG;
            }
    }

private:
    double sr = 48000.0;
    float lastKey = -1.f;
    Biquad pre[2][2], post[2][5], low[2];
    OnePole inter[2][3];
    float dcState[2][3] {}, dcX[2][3] {};
    juce::dsp::Oversampling<float> os { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
};

// ------------------------------------------------------------------------------------------------
// Speaker cabinet: speaker + mic response as a filter chain, plus a short room.
class CabSim
{
public:
    struct Model { float hp, lp, f1, g1, q1, f2, g2, q2, f3, g3, q3; };
    static const Model& model (int m)
    {
        static const Model list[] {
            { 75.f, 5500.f,  120.f, 3.f, 1.0f,  800.f, -2.f, 1.2f, 2200.f, 4.f, 1.5f },   // 1x12 Open Back
            { 70.f, 6000.f,  110.f, 4.f, 1.2f, 1000.f, -1.f, 1.0f, 2600.f, 5.f, 1.8f },   // 2x12 Blue
            { 80.f, 5000.f,  100.f, 5.f, 1.2f,  500.f, -3.f, 1.0f, 2800.f, 4.f, 1.6f },   // 4x12 British
            { 70.f, 5500.f,   90.f, 4.f, 1.0f, 1500.f,  2.f, 1.0f, 3500.f, 5.f, 2.0f },   // 4x12 Modern
            { 90.f, 6500.f,  150.f, 2.f, 1.0f, 1200.f,  3.f, 0.8f, 3000.f, 2.f, 1.5f },   // Tweed Combo
            { 40.f, 3500.f,   70.f, 4.f, 1.0f,  500.f, -2.f, 1.0f, 1500.f, 2.f, 1.2f },   // 1x15 Bass
            { 45.f, 4500.f,   80.f, 3.f, 1.0f,  600.f, -1.f, 1.0f, 1800.f, 3.f, 1.4f },   // 4x10 Bass
            { 35.f, 4000.f,   60.f, 5.f, 1.0f,  400.f, -2.f, 1.0f, 1200.f, 2.f, 1.0f },   // 8x10 Bass
            { 35.f, 16000.f, 100.f, 1.f, 0.7f, 1000.f,  0.f, 1.0f, 4000.f, 1.f, 1.0f },   // Full-Range Keys
            { 300.f, 3500.f, 1000.f, 6.f, 0.8f, 2000.f, 2.f, 1.0f,  600.f, 2.f, 1.0f },   // Vintage Radio
        };
        return list[juce::jlimit (0, 9, m)];
    }

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (auto& d : room) d.init ((int) (sr * 0.06));
        lastKey = -1.f;
    }

    void process (float* L, float* R, int n, int m, float mic, float roomAmt, float levelDb)
    {
        const auto& md = model (m);
        const float key = (float) m * 10.f + mic;
        if (key != lastKey)
        {
            lastKey = key;
            const double lpMul = std::exp2 (0.6 - 1.5 * mic);
            for (int c = 0; c < 2; ++c)
            {
                f[c][0].highpass (md.hp, 0.8, sr);
                f[c][1].lowpass (std::min (md.lp * lpMul, sr * 0.45), 0.9, sr);
                f[c][2].peak (md.f1, md.q1, md.g1, sr);
                f[c][3].peak (md.f2, md.q2, md.g2, sr);
                f[c][4].peak (md.f3, md.q3, md.g3 * (1.2 - mic * 0.8), sr);
                f[c][5].lowpass (std::min (md.lp * lpMul * 1.3, sr * 0.45), 0.6, sr);
                roomLp[c].setLowpass (4000.0, sr);
            }
        }
        const float g = dbToGain (levelDb) * 1.2f;
        static const float tapsL[] { 0.0071f, 0.0133f, 0.0197f, 0.0291f, 0.0389f };
        static const float tapsR[] { 0.0089f, 0.0151f, 0.0227f, 0.0317f, 0.0433f };
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            for (int s = 0; s < 6; ++s) { l = f[0][s].process (l); r = f[1][s].process (r); }
            if (roomAmt > 0.001f)
            {
                room[0].push (l); room[1].push (r);
                float el = 0.f, er = 0.f, gg = 0.55f;
                for (int t = 0; t < 5; ++t) { el += room[1].read (tapsL[t] * (float) sr) * gg; er += room[0].read (tapsR[t] * (float) sr) * gg; gg *= 0.72f; }
                l += roomLp[0].lp (el) * roomAmt; r += roomLp[1].lp (er) * roomAmt;
            }
            L[i] = l * g; R[i] = r * g;
        }
    }

private:
    double sr = 48000.0;
    float lastKey = -1.f;
    Biquad f[2][6];
    Delay room[2];
    OnePole roomLp[2];
};
