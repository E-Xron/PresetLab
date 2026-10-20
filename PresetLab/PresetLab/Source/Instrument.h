#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>
#include "Dsp.h"
#include "Params.h"

// The instrument engine: one voice type that can be a plucked string, a piano, an organ, an FM electric piano,
// a virtual analog synth, mallets, a wind instrument or a drum kit.
struct SoundSettings
{
    int inst = pl::Piano, kit = 0, wave = 0;
    float tone = 0.6f, charA = 0.5f, charB = 0.5f;
    float atk = 0.002f, dec = 1.5f, sus = 0.8f, rel = 0.35f;
    float cutoff = 20000.f, reso = 0.1f, fenv = 0.f;
    float detune = 0.2f, vibrato = 0.f, width = 0.5f, velSens = 0.7f;
    float pitch = 0.f;          // octave + fine, in semitones
};

class PlSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

class PlVoice : public juce::SynthesiserVoice
{
public:
    explicit PlVoice (const SoundSettings& s, int index);

    bool canPlaySound (juce::SynthesiserSound*) override { return true; }
    void startNote (int note, float velocity, juce::SynthesiserSound*, int pitchWheel) override;
    void stopNote (float velocity, bool allowTailOff) override;
    void pitchWheelMoved (int v) override { bend = (float) (v - 8192) / 8192.f * 2.f; }
    void controllerMoved (int cc, int v) override { if (cc == 1) modWheel = (float) v / 127.f; }
    void renderNextBlock (juce::AudioBuffer<float>&, int start, int num) override;
    void setCurrentPlaybackSampleRate (double) override;

    int  drumPiece() const { return isVoiceActive() && model == pl::Drums ? piece : -1; }
    void choke() { choking = true; }

    enum Piece { Kick, Snare, Rim, Clap, HatClosed, HatPedal, HatOpen, Tom, Crash, Ride, RideBell, Tamb, Cowbell };

private:
    static constexpr int kChunk = 64;
    static constexpr int kMaxPartials = 24;

    void renderChunk (float* L, float* R, int n);
    void renderString (float* L, float* R, int n);
    void renderPiano (float* L, float* R, int n);
    void renderOrgan (float* L, float* R, int n);
    void renderEPiano (float* L, float* R, int n);
    void renderBell (float* L, float* R, int n);
    void renderMallets (float* L, float* R, int n);
    void renderVA (float* L, float* R, int n);
    void renderWind (float* L, float* R, int n);
    void renderDrum (float* L, float* R, int n);
    void startDrum();

    double freqNow() const;
    float vibratoMult();

    const SoundSettings& set;
    int voiceIndex;
    double sr = 48000.0;

    // captured at note start
    int model = 0, note = 60, kit = 0, wave = 0, piece = 0;
    float vel = 1.f, velGain = 1.f, bend = 0.f, modWheel = 0.f, pan = 0.f, panSpread = 0.f;
    bool filterOn = false, struck = false, choking = false, ending = false;
    float chokeGain = 1.f;
    double t = 0.0;            // seconds since note on
    float vibPhase = 0.f;

    juce::ADSR ampEnv, filtEnv;
    dsp1::Svf svf[2];
    int svfCount = 0;
    dsp1::Rng rng;

    // ---- plucked string
    dsp1::Delay line;
    std::vector<float> excite;
    int exciteLen = 0, excitePos = 0;
    float ksLoss = 0.99f, ksDamp = 0.3f, ksLp = 0.f, ksDc = 0.f, ksDcX = 0.f, ksPickup = 0.2f;
    dsp1::Biquad body[4];
    float bodyGain[4] {};

    // ---- additive / modal / FM partial banks (complex phasors)
    struct Partial { double c = 1, s = 0, dc = 1, ds = 0; float amp = 0, amp2 = 0, d1 = 1, d2 = 1; double ratio = 1; };
    std::array<Partial, kMaxPartials * 2> parts;
    int numParts = 0;
    float hammer = 0.f, hammerDecay = 0.f;
    dsp1::OnePole noiseLp, noiseLp2;
    void setPhasorFreq (Partial&, double hz);

    // ---- FM
    double phC = 0, phM = 0, phT = 0, phC2 = 0, phM2 = 0;
    float fmIndex = 0, fmIndexDecay = 1, ampDecay = 1, ampLevel = 1, tineLevel = 0, tineDecay = 1;

    // ---- virtual analog
    std::array<double, 8> oscPh {};
    std::array<float, 8> oscDet {}, oscPan {};
    int numOsc = 1;
    double subPh = 0, lfoPh = 0;
    dsp1::Biquad breathBp;

    // ---- drums
    struct DrumState
    {
        double ph[6] {}; float fStart = 60, fEnd = 50, pitchTau = 0.03f, ampDecay = 0.999f, amp = 1.f;
        float noiseAmp = 0.f, noiseDecay = 0.99f, clickAmp = 0.f, clickDecay = 0.9f, metalAmp = 0.f, metalDecay = 0.99f;
        float tone2 = 0.f, tone2Ratio = 1.6f, drive = 0.f, crush = 0.f; int crushHold = 1, crushCount = 0; float crushVal = 0.f;
        double fMetal[6] {}; int clapBursts = 0;
    } dr;
    dsp1::Biquad drumBp, drumHp, drumLp;
};

class PlSynth : public juce::Synthesiser
{
public:
    void noteOn (int ch, int note, float velocity) override;
    const SoundSettings* settings = nullptr;
};
