#include "Instrument.h"

using namespace dsp1;

namespace
{
    // General MIDI drum map; notes outside 35-59 repeat the C1-B1 layout in every octave
    int pieceForNote (int n, float& tomPitch)
    {
        tomPitch = 0.f;
        if (n < 35 || n > 59) n = 36 + ((n - 36) % 12 + 12) % 12;
        switch (n)
        {
            case 35: case 36: return PlVoice::Kick;
            case 37: return PlVoice::Rim;
            case 38: case 40: return PlVoice::Snare;
            case 39: return PlVoice::Clap;
            case 42: return PlVoice::HatClosed;
            case 44: return PlVoice::HatPedal;
            case 46: return PlVoice::HatOpen;
            case 41: tomPitch = 0.f; return PlVoice::Tom;
            case 43: tomPitch = 0.2f; return PlVoice::Tom;
            case 45: tomPitch = 0.4f; return PlVoice::Tom;
            case 47: tomPitch = 0.6f; return PlVoice::Tom;
            case 48: tomPitch = 0.8f; return PlVoice::Tom;
            case 50: tomPitch = 1.0f; return PlVoice::Tom;
            case 49: case 52: case 55: case 57: return PlVoice::Crash;
            case 51: case 59: return PlVoice::Ride;
            case 53: return PlVoice::RideBell;
            case 54: return PlVoice::Tamb;
            case 56: return PlVoice::Cowbell;
            default: tomPitch = 0.5f; return PlVoice::Tom;
        }
    }

    // loudness trims so every instrument sits at a similar level
    float modelTrim (int m)
    {
        static const float t[pl::kNumInst] { 1.1f, 1.4f, 1.3f, 0.36f, 0.23f, 0.55f, 0.11f, 0.33f, 0.4f, 0.42f, 0.45f, 0.38f, 0.55f, 0.27f, 0.7f };
        return t[juce::jlimit (0, pl::kNumInst - 1, m)];
    }

    inline void panGains (float p, float& l, float& r)
    {
        p = clampf (p, -1.f, 1.f);
        const float a = (p + 1.f) * (float) kPi * 0.25f;
        l = 1.41421356f * std::cos (a); r = 1.41421356f * std::sin (a);
    }
}

PlVoice::PlVoice (const SoundSettings& s, int index) : set (s), voiceIndex (index)
{
    rng.seed (0x9e3779b9u * (uint32_t) (index + 1));
}

void PlVoice::setCurrentPlaybackSampleRate (double newRate)
{
    SynthesiserVoice::setCurrentPlaybackSampleRate (newRate);
    if (newRate <= 0) return;
    sr = newRate;
    ampEnv.setSampleRate (sr);
    filtEnv.setSampleRate (sr);
    line.init ((int) (sr / 18.0) + 64);
    excite.assign ((size_t) (sr / 18.0) + 64, 0.f);
}

double PlVoice::freqNow() const
{
    return juce::jlimit (20.0, sr * 0.45, 440.0 * std::exp2 (((double) note - 69.0 + set.pitch + bend) / 12.0));
}

float PlVoice::vibratoMult()
{
    float depth = set.vibrato * 0.5f + modWheel * 0.4f;
    if (model == pl::Strings) depth += 0.12f;
    if (model == pl::Flute) depth += 0.08f;
    if (depth <= 0.f) return 1.f;
    const float ramp = (float) juce::jlimit (0.0, 1.0, (t - 0.15) / 0.5);
    vibPhase += (float) (5.4 * kChunk / sr);
    if (vibPhase > 1.f) vibPhase -= 1.f;
    return std::exp2 (depth * ramp * std::sin (vibPhase * (float) kTwoPi) / 12.f);
}

void PlVoice::setPhasorFreq (Partial& p, double hz)
{
    const double w = kTwoPi * hz / sr;
    p.dc = std::cos (w); p.ds = std::sin (w);
}

void PlVoice::startNote (int n, float velocity, juce::SynthesiserSound*, int pitchWheel)
{
    model = set.inst; kit = set.kit; wave = set.wave; note = n; vel = velocity;
    bend = (float) (pitchWheel - 8192) / 8192.f * 2.f;
    const float vs = set.velSens;
    velGain = (1.f - vs) + vs * std::pow (vel, 1.5f);
    struck = pl::isStruck (model);
    choking = false; ending = false; chokeGain = 1.f;
    t = 0.0; vibPhase = 0.f;
    for (auto& s : svf) s.reset();

    const float decMult = set.dec / 1.5f;
    juce::ADSR::Parameters ap, fp;
    if (struck) { ap = { set.atk, 0.1f, 1.f, set.rel }; fp = { set.atk, set.dec * 0.5f, 0.f, set.rel }; }
    else        { ap = { set.atk, set.dec, set.sus, set.rel }; fp = { set.atk, set.dec, set.sus, set.rel }; }
    if (model == pl::Drums) ap = { 0.0005f, 0.1f, 1.f, 0.02f };
    ampEnv.setParameters (ap); filtEnv.setParameters (fp);
    ampEnv.reset(); filtEnv.reset();
    ampEnv.noteOn(); filtEnv.noteOn();

    filterOn = model != pl::Drums && (set.cutoff < 19000.f || set.reso > 0.05f || model == pl::Brass || model == pl::Pad);
    pan = 0.f;
    if (model == pl::Piano || model == pl::Mallets || model == pl::EPiano) pan = (float) (n - 64) / 48.f * set.width * 0.6f;
    pan += rng.next() * 0.06f * set.width;
    panSpread = set.width;

    const double f = freqNow();
    switch (model)
    {
        case pl::AcousticGtr: case pl::ElectricGtr: case pl::BassGtr:
        {
            const double P = sr / f;
            const float hard = juce::jlimit (0.f, 1.f, set.tone * 0.5f + vel * 0.5f + (model == pl::BassGtr ? (set.charA - 0.5f) * 0.6f : 0.f));
            float beta;
            double t60;
            if (model == pl::AcousticGtr)      { ksDamp = 0.08f + (1.f - set.tone) * 0.45f; beta = 0.06f + 0.34f * set.charA; t60 = 3.2 * std::pow (196.0 / f, 0.6); }
            else if (model == pl::ElectricGtr) { ksDamp = 0.06f + (1.f - set.tone) * 0.45f; beta = 0.13f; t60 = 6.0 * std::pow (196.0 / f, 0.5); }
            else                               { ksDamp = 0.18f + (1.f - set.tone) * 0.5f;  beta = 0.28f - 0.17f * set.charA; t60 = 7.0 * std::pow (55.0 / f, 0.5); }
            ksDamp *= (1.f - 0.35f * vel);
            t60 *= juce::jlimit (0.05f, 6.f, decMult);
            ksLoss = (float) std::pow (10.0, -3.0 * P / (sr * t60));

            exciteLen = juce::jlimit (8, (int) excite.size() - 1, (int) std::round (P));
            const double fc = 300.0 + std::pow (0.3 + 0.7 * hard, 2.0) * 9000.0;
            noiseLp.setLowpass (fc, sr); noiseLp.reset();
            const int D = juce::jmax (1, (int) std::round (beta * P));
            for (int i = 0; i < exciteLen; ++i) excite[(size_t) i] = noiseLp.lp (rng.next());
            for (int i = exciteLen - 1; i >= D; --i) excite[(size_t) i] -= excite[(size_t) (i - D)];
            float peak = 1e-6f; for (int i = 0; i < exciteLen; ++i) peak = std::max (peak, std::abs (excite[(size_t) i]));
            for (int i = 0; i < exciteLen; ++i) excite[(size_t) i] *= 0.6f / peak;
            excitePos = 0;
            line.clear(); ksLp = 0.f; ksDc = 0.f; ksDcX = 0.f;
            ksPickup = 0.06f + 0.24f * set.charA;

            for (auto& b : body) b.reset();
            for (auto& g : bodyGain) g = 0.f;
            if (model == pl::AcousticGtr)
            {
                body[0].bandpass (102, 4.0, sr); body[1].bandpass (215, 6.0, sr); body[2].bandpass (440, 3.0, sr); body[3].bandpass (2900, 1.2, sr);
                const float k = set.charB * 1.4f;
                bodyGain[0] = 1.0f * k; bodyGain[1] = 0.8f * k; bodyGain[2] = 0.5f * k; bodyGain[3] = 0.25f * k;
            }
            else if (model == pl::BassGtr)
            {
                body[0].bandpass (72, 1.4, sr); bodyGain[0] = set.charB * 1.6f;
                body[1].bandpass (900, 1.0, sr); bodyGain[1] = set.charA * 0.25f;
            }
            break;
        }
        case pl::Piano:
        {
            const double B = juce::jlimit (0.00004, 0.0025, 0.00025 * std::pow (2.0, (n - 60) / 22.0));
            const float h = juce::jlimit (0.f, 1.f, set.charA * 0.55f + vel * 0.45f);
            const float slope = juce::jlimit (0.55f, 2.6f, 2.4f - 1.5f * h - set.tone * 0.6f);
            const double tBase = 30.0 * std::pow (55.0 / f, 0.75) * juce::jlimit (0.05f, 6.f, decMult);
            const double det = (0.3 + set.charB * 2.2) / 1200.0 * kTwoPi;      // string pair detune (cents)
            numParts = 0;
            for (int k = 1; k <= kMaxPartials; ++k)
            {
                const double fk = k * f * std::sqrt (1.0 + B * k * k);
                if (fk > sr * 0.45) break;
                float a = std::pow ((float) k, -slope) * (0.25f + 0.75f * std::abs (std::sin ((float) kPi * (float) k / 7.6f)));
                if (fk > 6000.0) a *= (float) (6000.0 / fk);
                const double tk = tBase / (1.0 + 0.22 * (k - 1));
                for (int s = 0; s < 2; ++s)
                {
                    auto& p = parts[(size_t) numParts++];
                    p.ratio = fk / f * (1.0 + (s == 0 ? -det : det) * 0.16);
                    const double ph0 = rng.uni() * kTwoPi * 0.1; p.c = std::cos (ph0); p.s = std::sin (ph0);
                    p.amp = a * 0.62f; p.amp2 = a * 0.38f;
                    p.d1 = decayCoef (tk * 0.22, sr); p.d2 = decayCoef (tk, sr);
                }
            }
            hammer = 0.12f * vel * (0.4f + h); hammerDecay = decayCoef (0.03, sr);
            noiseLp.setLowpass (f * 2.5 + 600.0, sr); noiseLp.reset();
            break;
        }
        case pl::Organ:
        {
            static const float reg[5][9] {
                { 0, 0, 8, 6, 0, 0, 0, 0, 0 }, { 8, 8, 8, 0, 0, 0, 0, 0, 0 }, { 8, 8, 8, 8, 0, 0, 0, 0, 4 },
                { 8, 8, 8, 8, 6, 6, 0, 0, 6 }, { 8, 8, 8, 8, 8, 8, 8, 8, 8 } };
            static const double ratios[9] { 0.5, 1.5, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 8.0 };
            const float x = set.charA * 4.f; const int i0 = juce::jmin (3, (int) x); const float fr = x - (float) i0;
            numParts = 0;
            for (int k = 0; k < 9; ++k)
            {
                if (f * ratios[k] > sr * 0.45) continue;
                const float lvl = lerp (reg[i0][k], reg[i0 + 1][k], fr);
                if (lvl <= 0.05f) continue;
                auto& p = parts[(size_t) numParts++];
                p.ratio = ratios[k]; p.c = 1; p.s = 0;
                p.amp = std::pow (10.f, (lvl - 8.f) * 3.f / 20.f) * (k >= 4 ? 0.35f + 0.65f * set.tone : 1.f);
                p.amp2 = 0.f; p.d1 = 1.f; p.d2 = 1.f;
            }
            if (set.charB > 0.02f && f * 3.0 < sr * 0.45)     // percussion on the 3rd harmonic
            {
                auto& p = parts[(size_t) numParts++];
                p.ratio = 3.0; p.c = 1; p.s = 0; p.amp = 0.f; p.amp2 = set.charB * 1.4f; p.d1 = 1.f; p.d2 = decayCoef (0.45, sr);
            }
            hammer = 0.05f + set.charB * 0.15f; hammerDecay = decayCoef (0.006, sr);   // key click
            noiseLp.setLowpass (2500.0, sr); noiseLp.reset();
            break;
        }
        case pl::EPiano:
        {
            phC = phM = phT = 0.0;
            fmIndex = (0.25f + 2.6f * set.charA) * (0.25f + 0.75f * vel);
            fmIndexDecay = decayCoef (0.9, sr);
            ampLevel = 1.f; ampDecay = decayCoef (5.0 * std::pow (262.0 / f, 0.5) * juce::jlimit (0.05f, 6.f, decMult), sr);
            tineLevel = 0.22f * set.charA * vel; tineDecay = decayCoef (0.05, sr);
            noiseLp2.setLowpass (1200.0 + set.tone * set.tone * 12000.0, sr); noiseLp2.reset();
            break;
        }
        case pl::Bell:
        {
            static const double r[] { 1.0, 1.4, 2.0, 2.76, 3.5, 5.19, 7.0 };
            const float x = set.charA * 6.f; const int i0 = juce::jmin (5, (int) x);
            const double ratio = r[i0] + (r[i0 + 1] - r[i0]) * (x - (float) i0);
            phC = phM = phC2 = phM2 = 0.0;
            phT = ratio;                                           // stores the modulator ratio
            fmIndex = (0.6f + 5.f * set.tone) * (0.3f + 0.7f * vel);
            fmIndexDecay = decayCoef (0.15 + 1.6 * set.dec / 1.5, sr);
            ampLevel = 1.f; ampDecay = decayCoef (set.dec * 1.3 * std::pow (440.0 / f, 0.3), sr);
            tineLevel = set.charB;                                 // second operator pair level
            break;
        }
        case pl::Mallets:
        {
            static const float rat[3][4] { { 1.f, 3.99f, 10.65f, 0.f }, { 1.f, 4.0f, 10.0f, 0.f }, { 1.f, 2.756f, 5.404f, 8.933f } };
            static const float amp[3][4] { { 1.f, 0.45f, 0.15f, 0.f }, { 1.f, 0.3f, 0.1f, 0.f }, { 1.f, 0.5f, 0.3f, 0.15f } };
            static const float tm[3][4]  { { 1.f, 0.3f, 0.12f, 0.1f }, { 1.f, 0.5f, 0.3f, 0.2f }, { 1.f, 0.6f, 0.4f, 0.3f } };
            const double base[3] { 0.9 * std::pow (262.0 / f, 0.5), 4.0 * std::pow (262.0 / f, 0.3), 2.5 * std::pow (523.0 / f, 0.3) };
            const float x = set.charA * 2.f; const int i0 = juce::jmin (1, (int) x); const float fr = x - (float) i0;
            const double tb = (base[i0] + (base[i0 + 1] - base[i0]) * fr) * juce::jlimit (0.05f, 6.f, decMult);
            numParts = 0;
            for (int k = 0; k < 4; ++k)
            {
                const float ra = lerp (rat[i0][k], rat[i0 + 1][k], fr), am = lerp (amp[i0][k], amp[i0 + 1][k], fr);
                if (ra <= 0.f || am <= 0.001f || f * ra > sr * 0.45) continue;
                auto& p = parts[(size_t) numParts++];
                p.ratio = ra; p.c = 1; p.s = 0;
                p.amp = am * (k == 0 ? 0.8f + 0.4f * set.charB : (0.4f + 0.8f * set.tone)) * (k == 0 ? 1.f : vel);
                p.amp2 = 0.f;
                p.d1 = decayCoef (tb * lerp (tm[i0][k], tm[i0 + 1][k], fr) * (k == 0 ? 0.6 + 1.4 * set.charB : 1.0), sr); p.d2 = 1.f;
            }
            hammer = 0.25f * set.tone * vel; hammerDecay = decayCoef (0.006, sr);
            noiseLp.setLowpass (5000.0, sr); noiseLp.reset();
            break;
        }
        case pl::Flute:
        {
            phC = 0.0;
            breathBp.reset(); breathBp.bandpass (juce::jmin (f * 2.0, sr * 0.4), 1.2, sr);
            hammer = 0.5f * set.charA + 0.1f; hammerDecay = decayCoef (0.08, sr);
            break;
        }
        case pl::Drums:
            startDrum();
            break;
        default:   // virtual analog
        {
            const bool supersaw = model != pl::Strings && wave == 4;
            numOsc = model == pl::SynthBass ? 1 : model == pl::Synth ? 2 : model == pl::Pad ? 4 : model == pl::Strings ? 5 : 2;
            if (supersaw) numOsc = 7;
            float spread = model == pl::Synth ? set.detune * 30.f : model == pl::Pad ? 6.f + set.detune * 30.f
                         : model == pl::Strings ? 6.f + set.charA * 22.f + set.detune * 10.f : model == pl::Brass ? set.detune * 14.f : set.detune * 20.f;
            if (supersaw) spread = 8.f + set.detune * 40.f;
            for (int k = 0; k < numOsc; ++k)
            {
                const float pos = numOsc > 1 ? (float) k / (float) (numOsc - 1) * 2.f - 1.f : 0.f;
                oscDet[(size_t) k] = std::exp2 (spread * pos / 1200.f);
                oscPan[(size_t) k] = pos * set.width;
                oscPh[(size_t) k] = rng.uni();
            }
            subPh = 0.0; lfoPh = rng.uni();
            breathBp.reset(); breathBp.bandpass (model == pl::Brass ? 1500.0 : 3200.0, 1.0, sr);
            noiseLp2.setLowpass (1500.0 + set.tone * set.tone * 14000.0, sr); noiseLp2.reset();
            noiseLp.setLowpass (1500.0 + set.tone * set.tone * 14000.0, sr); noiseLp.reset();
            break;
        }
    }
}

void PlVoice::startDrum()
{
    float tomPitch;
    piece = pieceForNote (note, tomPitch);
    dr = DrumState();
    const float tune = std::exp2 ((set.charA - 0.5f) * 1.2f), snap = set.charB, bright = set.tone;
    const float dm = juce::jlimit (0.1f, 4.f, set.dec / 1.5f);
    const bool k808 = kit == 2, k909 = kit == 3, lofi = kit == 4, electro = kit == 5, rock = kit == 1;
    auto D = [&] (double seconds) { return decayCoef (seconds * dm, sr); };
    const double metal[6] { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };
    for (int i = 0; i < 6; ++i) dr.fMetal[i] = metal[i] * (k808 ? 1.0 : k909 ? 1.25 : 1.6) * tune;
    pan = 0.f;

    switch (piece)
    {
        case Kick:
            if (k808)      { dr.fStart = 110; dr.fEnd = 46; dr.pitchTau = 0.05f; dr.ampDecay = D (1.4); dr.clickAmp = 0.15f; dr.drive = 0.25f; }
            else if (k909) { dr.fStart = 240; dr.fEnd = 52; dr.pitchTau = 0.022f; dr.ampDecay = D (0.45); dr.clickAmp = 0.6f; dr.noiseAmp = 0.15f; dr.drive = 0.4f; }
            else if (electro) { dr.fStart = 260; dr.fEnd = 48; dr.pitchTau = 0.014f; dr.ampDecay = D (0.3); dr.clickAmp = 0.5f; dr.drive = 0.6f; }
            else           { dr.fStart = rock ? 190.f : 160.f; dr.fEnd = rock ? 50.f : 56.f; dr.pitchTau = 0.03f; dr.ampDecay = D (rock ? 0.5 : 0.35); dr.clickAmp = rock ? 0.7f : 0.45f; dr.noiseAmp = 0.1f; dr.drive = rock ? 0.3f : 0.1f; }
            dr.fStart *= tune; dr.fEnd *= tune;
            dr.clickAmp *= 0.5f + snap; dr.clickDecay = decayCoef (0.004, sr); dr.noiseDecay = decayCoef (0.02, sr);
            drumLp.lowpass (2500 + bright * 6000, 0.7, sr);
            break;
        case Snare:
            dr.fStart = dr.fEnd = (k808 ? 238.f : k909 ? 200.f : 185.f) * tune; dr.tone2 = 0.5f; dr.tone2Ratio = k808 ? 2.0f : 1.65f;
            dr.amp = k808 ? 0.9f : 0.6f; dr.ampDecay = D (k808 ? 0.16 : 0.1);
            dr.noiseAmp = (k808 ? 0.5f : 0.9f) * (0.4f + snap); dr.noiseDecay = D (rock ? 0.24 : k909 ? 0.2 : 0.17);
            dr.clickAmp = 0.3f; dr.clickDecay = decayCoef (0.003, sr); dr.drive = rock ? 0.3f : 0.1f;
            drumBp.bandpass ((k909 ? 6000.0 : 4500.0) * (0.6 + bright * 0.8), 0.6, sr); drumHp.highpass (k808 ? 1800.0 : 900.0, 0.7, sr);
            pan = 0.05f;
            break;
        case Rim:
            dr.fStart = dr.fEnd = 1650.f * tune; dr.amp = 0.5f; dr.ampDecay = D (0.025);
            dr.noiseAmp = 0.5f; dr.noiseDecay = D (0.02); drumBp.bandpass (2200, 2.0, sr); drumHp.highpass (500, 0.7, sr);
            pan = 0.05f;
            break;
        case Clap:
            dr.amp = 0.f; dr.noiseAmp = 1.0f; dr.noiseDecay = D (0.16 + snap * 0.2); dr.clapBursts = 1;
            drumBp.bandpass (1150.0 * tune * (0.7 + bright * 0.6), 1.4, sr); drumHp.highpass (600, 0.7, sr);
            pan = -0.05f;
            break;
        case HatClosed: case HatPedal: case HatOpen:
        {
            const double len = piece == HatOpen ? 0.5 : piece == HatPedal ? 0.09 : 0.055;
            dr.amp = 0.f;
            const bool synthy = k808 || k909 || electro;
            dr.metalAmp = synthy ? 0.8f : 0.4f; dr.metalDecay = D (len * (0.6 + snap * 0.8));
            dr.noiseAmp = synthy ? 0.25f : 0.6f; dr.noiseDecay = D (len * (0.6 + snap * 0.8));
            drumBp.bandpass (10000.0 * (0.7 + bright * 0.5), 0.8, sr); drumHp.highpass (7000.0 * (0.7 + bright * 0.4), 0.7, sr);
            pan = 0.35f * set.width;
            break;
        }
        case Crash: case Ride: case RideBell:
        {
            dr.amp = 0.f;
            dr.metalAmp = piece == RideBell ? 0.9f : 0.5f;
            dr.metalDecay = D (piece == Crash ? 1.6 : piece == Ride ? 2.4 : 1.8);
            dr.noiseAmp = piece == Crash ? 0.7f : piece == Ride ? 0.18f : 0.05f; dr.noiseDecay = D (piece == Crash ? 1.3 : 0.6);
            if (piece != Crash) { dr.fStart = dr.fEnd = (piece == RideBell ? 760.f : 520.f) * tune; dr.amp = piece == RideBell ? 0.35f : 0.06f; dr.ampDecay = D (1.6); }
            drumBp.bandpass (piece == RideBell ? 3500.0 : 8000.0 * (0.7 + bright * 0.5), 0.6, sr); drumHp.highpass (piece == RideBell ? 1500.0 : 4000.0, 0.7, sr);
            pan = (piece == Crash ? -0.45f : 0.45f) * set.width;
            break;
        }
        case Tom:
        {
            const float base = (k808 ? 70.f : 85.f) * std::exp2 (tomPitch * 1.6f) * tune;
            dr.fStart = base * (k808 ? 1.25f : 1.6f); dr.fEnd = base; dr.pitchTau = 0.05f;
            dr.ampDecay = D ((k808 ? 0.7 : 0.45) - tomPitch * 0.15); dr.noiseAmp = k808 ? 0.f : 0.12f; dr.noiseDecay = D (0.05);
            dr.clickAmp = 0.2f * (0.5f + snap); dr.clickDecay = decayCoef (0.003, sr);
            drumLp.lowpass (3000, 0.7, sr);
            pan = (0.5f - tomPitch) * 0.7f * set.width;
            break;
        }
        case Tamb:
            dr.amp = 0.f; dr.noiseAmp = 0.7f; dr.noiseDecay = D (0.22); dr.metalAmp = 0.15f; dr.metalDecay = D (0.15);
            drumBp.bandpass (9000, 1.0, sr); drumHp.highpass (5500, 0.7, sr);
            pan = 0.5f * set.width;
            break;
        case Cowbell:
            dr.amp = 0.f; dr.metalAmp = 0.9f; dr.metalDecay = D (0.28);
            dr.fMetal[0] = 562.0 * tune; dr.fMetal[1] = 845.0 * tune;
            for (int i = 2; i < 6; ++i) dr.fMetal[i] = 0.0;
            drumBp.bandpass (2600, 1.2, sr); drumHp.highpass (400, 0.7, sr);
            pan = -0.3f * set.width;
            break;
        default: break;
    }
    if (lofi) { dr.crush = 1.f; dr.crushHold = juce::jmax (1, (int) (sr / 14000.0)); }
    drumBp.reset(); drumHp.reset(); drumLp.reset();
}

void PlVoice::stopNote (float, bool allowTailOff)
{
    if (! allowTailOff)
    {
        ampEnv.reset(); clearCurrentNote(); return;
    }
    if (model == pl::Drums) return;                        // drums are one-shots: they ring out on their own
    ampEnv.noteOff(); filtEnv.noteOff();
    if (model == pl::AcousticGtr || model == pl::ElectricGtr || model == pl::BassGtr)
        ksDamp = juce::jmin (0.85f, ksDamp + 0.3f);       // fingers mute the string
}

// ---------------------------------------------------------------------------------------------------------------------

void PlVoice::renderNextBlock (juce::AudioBuffer<float>& out, int start, int num)
{
    if (! isVoiceActive()) return;
    float L[kChunk], R[kChunk];
    const float trim = modelTrim (model) * velGain;
    float pl, pr; panGains (pan, pl, pr);

    while (num > 0)
    {
        const int n = std::min (num, kChunk);
        std::fill (L, L + n, 0.f); std::fill (R, R + n, 0.f);
        renderChunk (L, R, n);

        // filter + envelope
        if (filterOn)
        {
            const float fe = filtEnv.getNextSample();
            for (int i = 1; i < n; ++i) filtEnv.getNextSample();
            float fenv = set.fenv;
            double cut = set.cutoff;
            if (model == pl::Brass) { fenv += set.charA * 0.6f; cut = std::min (cut, 400.0 + set.tone * 3000.0); }
            if (model == pl::Pad)   cut *= std::exp2 (set.charB * 1.5 * std::sin (kTwoPi * lfoPh));
            cut *= std::exp2 (fenv * fe * 6.0 + ((double) note - 60.0) / 12.0 * 0.4);
            cut = juce::jlimit (30.0, sr * 0.45, cut);
            svf[0].set (cut, set.reso, sr); svf[1].set (cut, set.reso, sr);
            const float comp = 1.f / (1.f + set.reso * 0.8f);
            for (int i = 0; i < n; ++i) { L[i] = svf[0].tick (L[i]).lp * comp; R[i] = svf[1].tick (R[i]).lp * comp; }
        }
        else
            for (int i = 0; i < n; ++i) filtEnv.getNextSample();

        auto* oL = out.getWritePointer (0, start);
        auto* oR = out.getNumChannels() > 1 ? out.getWritePointer (1, start) : nullptr;
        for (int i = 0; i < n; ++i)
        {
            float e = ampEnv.getNextSample() * trim;
            if (choking) { chokeGain *= 0.995f; e *= chokeGain; }
            oL[i] += L[i] * e * pl;
            if (oR != nullptr) oR[i] += R[i] * e * pr;
        }

        t += n / sr;
        start += n; num -= n;
        if (! ampEnv.isActive() || ending || (choking && chokeGain < 0.001f))
        {
            clearCurrentNote(); ampEnv.reset(); return;
        }
    }
}

void PlVoice::renderChunk (float* L, float* R, int n)
{
    switch (model)
    {
        case pl::AcousticGtr: case pl::ElectricGtr: case pl::BassGtr: renderString (L, R, n); break;
        case pl::Piano:   renderPiano (L, R, n); break;
        case pl::Organ:   renderOrgan (L, R, n); break;
        case pl::EPiano:  renderEPiano (L, R, n); break;
        case pl::Bell:    renderBell (L, R, n); break;
        case pl::Mallets: renderMallets (L, R, n); break;
        case pl::Flute:   renderWind (L, R, n); break;
        case pl::Drums:   renderDrum (L, R, n); break;
        default:          renderVA (L, R, n); break;
    }
}

void PlVoice::renderString (float* L, float* R, int n)
{
    const double f = freqNow() * vibratoMult();
    const double P = sr / f, w = kTwoPi * f / sr;
    const double a = ksDamp;
    const double lpDelay = std::atan2 (a * std::sin (w), 1.0 - a * std::cos (w)) / w;   // one-pole phase delay at f
    const float rd = (float) juce::jmax (1.0, P - lpDelay - 1.0);
    const float g = model == pl::ElectricGtr && ! isKeyDown() && ! isSustainPedalDown() ? ksLoss * 0.995f : ksLoss;
    const float pick = (float) (ksPickup * P);
    const float bite = model == pl::ElectricGtr ? 1.f + set.charB * 2.f : 1.f;
    float energy = 0.f;

    for (int i = 0; i < n; ++i)
    {
        const float x = excitePos < exciteLen ? excite[(size_t) excitePos++] : 0.f;
        const float v = line.read (rd);
        ksLp = (float) ((1.0 - a) * v + a * ksLp);
        const float y = x + g * ksLp;
        line.push (y);
        float o = y;
        if (model == pl::ElectricGtr) o = (y - line.read (pick)) * 0.8f;
        // DC blocker
        const float dc = o - ksDcX + 0.995f * ksDc; ksDcX = o; ksDc = dc; o = dc;
        if (model == pl::ElectricGtr && bite > 1.f) o = fastTanh (o * bite) / bite * 1.3f;
        float b = 0.f;
        for (int k = 0; k < 4; ++k) if (bodyGain[k] > 0.f) b += body[k].process (o) * bodyGain[k];
        o += b;
        L[i] = o; R[i] = o;
        energy += std::abs (o);
    }
    if (excitePos >= exciteLen && energy < 1e-5f * (float) n && t > 0.05) ending = true;
}

void PlVoice::renderPiano (float* L, float* R, int n)
{
    const double f = freqNow();
    for (int k = 0; k < numParts; ++k) setPhasorFreq (parts[(size_t) k], f * parts[(size_t) k].ratio);
    const float w = 0.25f * panSpread;
    float level = 0.f;
    for (int i = 0; i < n; ++i)
    {
        float a = 0.f, b = 0.f;
        for (int k = 0; k < numParts; k += 2)
        {
            auto& p = parts[(size_t) k]; auto& q = parts[(size_t) k + 1];
            const double c1 = p.c * p.dc - p.s * p.ds; p.s = p.c * p.ds + p.s * p.dc; p.c = c1;
            const double c2 = q.c * q.dc - q.s * q.ds; q.s = q.c * q.ds + q.s * q.dc; q.c = c2;
            a += (float) p.s * (p.amp + p.amp2); p.amp *= p.d1; p.amp2 *= p.d2;
            b += (float) q.s * (q.amp + q.amp2); q.amp *= q.d1; q.amp2 *= q.d2;
        }
        const float h = noiseLp.lp (rng.next()) * hammer; hammer *= hammerDecay;
        L[i] = a * (1.f + w) + b * (1.f - w) + h;
        R[i] = a * (1.f - w) + b * (1.f + w) + h;
        level += std::abs (a) + std::abs (b);
    }
    for (int k = 0; k < numParts; ++k)
    {
        auto& p = parts[(size_t) k]; const double m = 1.5 - 0.5 * (p.c * p.c + p.s * p.s); p.c *= m; p.s *= m;
    }
    if (level < 1e-5f * (float) n && t > 0.1) ending = true;
}

void PlVoice::renderOrgan (float* L, float* R, int n)
{
    const double f = freqNow() * vibratoMult();
    for (int k = 0; k < numParts; ++k) setPhasorFreq (parts[(size_t) k], f * parts[(size_t) k].ratio);
    for (int i = 0; i < n; ++i)
    {
        float s = 0.f;
        for (int k = 0; k < numParts; ++k)
        {
            auto& p = parts[(size_t) k];
            const double c1 = p.c * p.dc - p.s * p.ds; p.s = p.c * p.ds + p.s * p.dc; p.c = c1;
            s += (float) p.s * (p.amp + p.amp2); p.amp2 *= p.d2;
        }
        s += (rng.next() - noiseLp.lp (rng.next())) * hammer; hammer *= hammerDecay;
        s = softClip (s * 0.6f) * 1.6f;
        L[i] = s; R[i] = s;
    }
    for (int k = 0; k < numParts; ++k)
    {
        auto& p = parts[(size_t) k]; const double m = 1.5 - 0.5 * (p.c * p.c + p.s * p.s); p.c *= m; p.s *= m;
    }
}

void PlVoice::renderEPiano (float* L, float* R, int n)
{
    const double f = freqNow() * vibratoMult();
    const double dC = f / sr, dT = std::min (f * 14.0, sr * 0.45) / sr;
    const float bark = 1.f + 3.f * set.charB * vel;
    const float barkNorm = 1.f / fastTanh (bark);
    for (int i = 0; i < n; ++i)
    {
        phC += dC; if (phC >= 1.0) phC -= 1.0;
        phT += dT; if (phT >= 1.0) phT -= 1.0;
        const float idx = fmIndex * (0.15f + 0.85f * ampLevel);
        float y = std::sin ((float) (kTwoPi * phC) + idx * std::sin ((float) (kTwoPi * phC)));
        y += tineLevel * std::sin ((float) (kTwoPi * phT)); tineLevel *= tineDecay;
        y = fastTanh (y * bark) * barkNorm + 0.12f * set.charB * vel * y * y;
        y = noiseLp2.lp (y) * ampLevel;
        ampLevel *= ampDecay; fmIndex *= fmIndexDecay;
        L[i] = y; R[i] = y;
    }
    if (ampLevel < 1e-4f) ending = true;
}

void PlVoice::renderBell (float* L, float* R, int n)
{
    const double f = freqNow() * vibratoMult();
    const double dC = f / sr, dM = std::min (f * phT, sr * 0.45) / sr;
    const double dC2 = f * 2.003 / sr, dM2 = std::min (f * 2.0 * phT * 1.01, sr * 0.45) / sr;
    const bool pair = tineLevel > 0.01f && f * 2.0 * phT < sr * 0.45;
    const float w = 0.4f * panSpread;
    for (int i = 0; i < n; ++i)
    {
        phC += dC; if (phC >= 1.0) phC -= 1.0;
        phM += dM; if (phM >= 1.0) phM -= 1.0;
        const float a = std::sin ((float) (kTwoPi * phC) + fmIndex * std::sin ((float) (kTwoPi * phM)));
        float b = 0.f;
        if (pair)
        {
            phC2 += dC2; if (phC2 >= 1.0) phC2 -= 1.0;
            phM2 += dM2; if (phM2 >= 1.0) phM2 -= 1.0;
            b = tineLevel * 0.6f * std::sin ((float) (kTwoPi * phC2) + fmIndex * 0.7f * std::sin ((float) (kTwoPi * phM2)));
        }
        L[i] = (a + b * (1.f + w)) * ampLevel;
        R[i] = (a + b * (1.f - w)) * ampLevel;
        ampLevel *= ampDecay; fmIndex *= fmIndexDecay;
    }
    if (ampLevel < 1e-4f) ending = true;
}

void PlVoice::renderMallets (float* L, float* R, int n)
{
    const double f = freqNow() * vibratoMult();
    for (int k = 0; k < numParts; ++k) setPhasorFreq (parts[(size_t) k], f * parts[(size_t) k].ratio);
    float level = 0.f;
    for (int i = 0; i < n; ++i)
    {
        float s = 0.f;
        for (int k = 0; k < numParts; ++k)
        {
            auto& p = parts[(size_t) k];
            const double c1 = p.c * p.dc - p.s * p.ds; p.s = p.c * p.ds + p.s * p.dc; p.c = c1;
            s += (float) p.s * p.amp; p.amp *= p.d1;
        }
        s += (rng.next() - noiseLp.lp (rng.next())) * hammer; hammer *= hammerDecay;
        L[i] = s; R[i] = s;
        level += std::abs (s);
    }
    for (int k = 0; k < numParts; ++k)
    {
        auto& p = parts[(size_t) k]; const double m = 1.5 - 0.5 * (p.c * p.c + p.s * p.s); p.c *= m; p.s *= m;
    }
    if (level < 1e-5f * (float) n && t > 0.05) ending = true;
}

void PlVoice::renderWind (float* L, float* R, int n)
{
    const double f = freqNow() * vibratoMult();
    const double d = f / sr;
    const float ob = set.charB, br = set.charA;
    const bool h3 = f * 3.0 < sr * 0.45, h4 = f * 4.0 < sr * 0.45;
    for (int i = 0; i < n; ++i)
    {
        phC += d; if (phC >= 1.0) phC -= 1.0;
        const float ph = (float) (kTwoPi * phC);
        float y = std::sin (ph) + ob * 0.5f * std::sin (2.f * ph);
        if (h3) y += ob * 0.28f * std::sin (3.f * ph);
        if (h4) y += set.tone * 0.08f * std::sin (4.f * ph);
        const float noise = breathBp.process (rng.next());
        y = y * (1.f + 0.15f * br * noise) + noise * (br * 0.5f + hammer);
        hammer *= hammerDecay;
        L[i] = y; R[i] = y;
    }
}

void PlVoice::renderVA (float* L, float* R, int n)
{
    const double f = freqNow() * vibratoMult();
    const bool supersaw = model != pl::Strings && wave == 4;
    const int w = model == pl::Strings || supersaw ? 0 : wave;
    const bool sub = (model == pl::SynthBass || model == pl::Synth) && set.charB > 0.01f;
    const bool shimmer = model == pl::Pad && set.charA > 0.01f && f * 2.0 < sr * 0.4;
    const bool noise = (model == pl::Strings || model == pl::Brass) && set.charB > 0.01f;
    const float drive = (model == pl::Synth || model == pl::SynthBass) && w != 5 ? 1.f + 5.f * set.charA : 1.f;
    const float driveNorm = drive > 1.f ? 1.f / fastTanh (drive) : 1.f;
    const float pw = 0.5f - 0.42f * set.charA;
    static const float waveNorm[] { 1.f, 0.577f, 1.f, 0.82f, 1.f, 0.62f };     // match every waveform to the saw's loudness
    const float norm = waveNorm[juce::jlimit (0, 5, w)] / std::sqrt ((float) numOsc);
    double dt[8];
    for (int k = 0; k < numOsc; ++k) dt[k] = f * oscDet[(size_t) k] / sr;
    float pg[8][2];
    for (int k = 0; k < numOsc; ++k) panGains (oscPan[(size_t) k], pg[k][0], pg[k][1]);
    lfoPh += 0.15 * n / sr; if (lfoPh > 1.0) lfoPh -= 1.0;

    for (int i = 0; i < n; ++i)
    {
        float l = 0.f, r = 0.f;
        for (int k = 0; k < numOsc; ++k)
        {
            double& p = oscPh[(size_t) k];
            p += dt[k]; if (p >= 1.0) p -= 1.0;
            const float ph = (float) p, d = (float) dt[k];
            float v;
            switch (w)
            {
                case 1:  v = (ph < 0.5f ? 1.f : -1.f) + polyBlep (ph, d) - polyBlep (std::fmod (ph + 0.5f, 1.f), d); break;
                case 2:  v = 1.f - 4.f * std::abs (ph - 0.5f); break;
                case 3:  v = std::sin (ph * (float) kTwoPi); break;
                case 5:  v = (ph < pw ? 1.f : -1.f) + polyBlep (ph, d) - polyBlep (std::fmod (ph + 1.f - pw, 1.f), d); break;
                default: v = 2.f * ph - 1.f - polyBlep (ph, d); break;
            }
            l += v * pg[k][0]; r += v * pg[k][1];
        }
        l *= norm; r *= norm;
        if (drive > 1.f) { l = fastTanh (l * drive) * driveNorm; r = fastTanh (r * drive) * driveNorm; }
        if (sub)
        {
            subPh += f * 0.5 / sr; if (subPh >= 1.0) subPh -= 1.0;
            const float s = std::sin ((float) (kTwoPi * subPh)) * set.charB * 0.9f;
            l += s; r += s;
        }
        if (shimmer)
        {
            subPh += f * 2.0 / sr; if (subPh >= 1.0) subPh -= 1.0;
            const float s = (2.f * (float) subPh - 1.f - polyBlep ((float) subPh, (float) (f * 2.0 / sr))) * set.charA * 0.35f;
            l += s; r -= s * 0.2f;
        }
        if (noise)
        {
            const float nz = breathBp.process (rng.next()) * set.charB * (model == pl::Brass ? 0.35f : 0.25f);
            l += nz; r += nz;
        }
        if (model == pl::Strings) { l = noiseLp2.lp (l); r = noiseLp.lp (r); }
        L[i] = l; R[i] = r;
    }
}

void PlVoice::renderDrum (float* L, float* R, int n)
{
    const double inv = 1.0 / sr;
    float level = 0.f;
    for (int i = 0; i < n; ++i)
    {
        const double tt = t + i * inv;
        float y = 0.f;
        if (dr.amp > 1e-5f)
        {
            const float f = dr.fEnd + (dr.fStart - dr.fEnd) * std::exp (-(float) tt / dr.pitchTau);
            dr.ph[0] += f * inv; if (dr.ph[0] >= 1.0) dr.ph[0] -= 1.0;
            float tone = std::sin ((float) (kTwoPi * dr.ph[0]));
            if (dr.tone2 > 0.f)
            {
                dr.ph[1] += f * dr.tone2Ratio * inv; if (dr.ph[1] >= 1.0) dr.ph[1] -= 1.0;
                tone = tone * (1.f - dr.tone2) + dr.tone2 * std::sin ((float) (kTwoPi * dr.ph[1]));
            }
            y += tone * dr.amp; dr.amp *= dr.ampDecay;
        }
        if (dr.clickAmp > 1e-5f) { y += drumLp.process (rng.next()) * dr.clickAmp; dr.clickAmp *= dr.clickDecay; }
        if (dr.metalAmp > 1e-5f)
        {
            float m = 0.f;
            for (int k = 0; k < 6; ++k)
            {
                if (dr.fMetal[k] <= 0.0) continue;
                dr.ph[k] += dr.fMetal[k] * inv; if (dr.ph[k] >= 1.0) dr.ph[k] -= 1.0;
                m += dr.ph[k] < 0.5 ? 1.f : -1.f;
            }
            m = drumBp.process (m * 0.25f);
            if (piece != Cowbell) m = drumHp.process (m);
            y += m * dr.metalAmp * 2.f; dr.metalAmp *= dr.metalDecay;
        }
        if (dr.noiseAmp > 1e-5f)
        {
            float nz = rng.next();
            if (piece == Kick || piece == Tom) nz = drumLp.process (nz) * 0.5f;
            else nz = drumHp.process (drumBp.process (nz) * 2.f);
            float env = dr.noiseAmp;
            if (dr.clapBursts && tt < 0.035) env = std::exp (-(float) std::fmod (tt, 0.0115) / 0.0035f) * 1.2f;
            y += nz * env;
            dr.noiseAmp *= dr.noiseDecay;
        }
        if (dr.drive > 0.f) y = fastTanh (y * (1.f + dr.drive * 3.f)) / (1.f + dr.drive);
        if (dr.crush > 0.f)
        {
            if (dr.crushCount++ % dr.crushHold == 0) dr.crushVal = std::round (y * 24.f) / 24.f;
            y = dr.crushVal;
        }
        L[i] = y; R[i] = y;
        level = std::max (level, std::abs (dr.amp) + dr.clickAmp + dr.metalAmp + dr.noiseAmp);
    }
    if (level < 2e-5f && t > 0.01) ending = true;
}

// ---------------------------------------------------------------------------------------------------------------------

void PlSynth::noteOn (int ch, int note, float velocity)
{
    if (settings != nullptr && settings->inst == pl::Drums)
    {
        float tp;
        const int pc = pieceForNote (note, tp);
        if (pc == PlVoice::HatClosed || pc == PlVoice::HatPedal)
            for (int i = 0; i < getNumVoices(); ++i)
                if (auto* v = dynamic_cast<PlVoice*> (getVoice (i)))
                    if (v->drumPiece() == PlVoice::HatOpen) v->choke();
    }
    Synthesiser::noteOn (ch, note, velocity);
}
