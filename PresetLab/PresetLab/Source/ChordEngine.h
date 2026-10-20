#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>
#include <vector>
#include "Params.h"

// Chord generator: turns one played note into a voiced, optionally strummed chord.
namespace chords
{
    struct Settings
    {
        bool on = false; int mode = 0, key = 0, scale = 0, type = 0, ext = 0, voicing = 0, inv = 0, dir = 0;
        float strumMs = 0.f, human = 0.f; bool bass = false;
    };

    inline const std::array<int, 7>& scale (int s)
    {
        static const std::array<int, 7> sc[] {
            { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 2, 3, 5, 7, 9, 10 },
            { 0, 2, 4, 5, 7, 9, 10 }, { 0, 2, 4, 6, 7, 9, 11 }, { 0, 1, 3, 5, 7, 8, 10 } };
        return sc[juce::jlimit (0, 6, s)];
    }

    inline std::vector<int> typeIntervals (int t)
    {
        static const std::vector<int> c[] {
            { 0, 4, 7 }, { 0, 3, 7 }, { 0, 7, 12 }, { 0, 2, 7 }, { 0, 5, 7 }, { 0, 4, 7, 10 }, { 0, 4, 7, 11 }, { 0, 3, 7, 10 },
            { 0, 4, 7, 9 }, { 0, 3, 7, 9 }, { 0, 4, 7, 10, 14 }, { 0, 4, 7, 11, 14 }, { 0, 3, 7, 10, 14 }, { 0, 4, 7, 14 }, { 0, 3, 6 }, { 0, 4, 8 } };
        return c[juce::jlimit (0, 15, t)];
    }

    // scale degree (0-6) a note falls on, snapping down to the scale; root gets the snapped note
    inline int degreeOf (int note, int key, int sc, int& root)
    {
        const auto& s = scale (sc);
        const int pc = ((note - key) % 12 + 12) % 12;
        int d = 0;
        for (int i = 0; i < 7; ++i) if (s[(size_t) i] <= pc) d = i;
        root = note - (pc - s[(size_t) d]);
        return d;
    }

    // chord tones (relative to the root) for a degree of the key
    inline std::vector<int> diatonic (int degree, int sc, int ext)
    {
        const auto& s = scale (sc);
        if (ext == 3) return { 0, 7, 12 };
        const int count = ext == 0 ? 3 : ext == 1 ? 4 : 5;
        std::vector<int> v;
        for (int j = 0; j < count; ++j)
        {
            const int d = degree + 2 * j;
            v.push_back (s[(size_t) (d % 7)] + 12 * (d / 7) - s[(size_t) degree]);
        }
        return v;
    }

    inline std::vector<int> voice (int root, std::vector<int> iv, const Settings& st)
    {
        std::vector<int> n;
        if (st.voicing == 4)          // guitar: low root and fifth under the chord, like a barre shape
        {
            const int fifth = iv.size() > 2 ? iv[2] : 7;
            n = { root - 12, root - 12 + fifth, root };
            for (size_t i = 1; i < iv.size(); ++i) n.push_back (root + iv[i]);
            if (iv.size() <= 3) n.push_back (root + 12);
        }
        else
        {
            for (int i : iv) n.push_back (root + i);
            std::sort (n.begin(), n.end());
            const int inv = std::min (st.inv, (int) n.size() - 1);
            for (int i = 0; i < inv; ++i) { n[0] += 12; std::sort (n.begin(), n.end()); }
            if (st.voicing == 1 && n.size() >= 3) n[1] += 12;
            else if (st.voicing == 2) { n[0] -= 12; if (n.size() >= 3) n.back() += 12; }
            else if (st.voicing == 3 && n.size() >= 3) n[n.size() - 2] -= 12;
        }
        std::sort (n.begin(), n.end());
        if (st.bass) { int b = root; while (b >= n.front()) b -= 12; n.insert (n.begin(), b); }
        std::vector<int> out;
        for (int x : n) { x = juce::jlimit (0, 127, x); if (std::find (out.begin(), out.end(), x) == out.end()) out.push_back (x); }
        std::sort (out.begin(), out.end());
        return out;
    }

    // the full chord for a played note under the current settings
    inline std::vector<int> forNote (int note, const Settings& st, bool forceKey = false, int forceDegree = -1)
    {
        if (st.mode == 1 && ! forceKey) return voice (note, typeIntervals (st.type), st);
        int root;
        int d = degreeOf (note, st.key, st.scale, root);
        if (forceDegree >= 0) { d = forceDegree; root = note; }
        return voice (root, diatonic (d, st.scale, st.ext), st);
    }

    inline juce::String name (int rootPc, const std::vector<int>& iv)
    {
        int mask = 0;
        for (int i : iv) mask |= 1 << (((i % 12) + 12) % 12);
        struct N { int m; const char* s; };
        static const N table[] {
            { 0x091, "" }, { 0x089, "m" }, { 0x049, "dim" }, { 0x111, "aug" }, { 0x081, "5" }, { 0x085, "sus2" }, { 0x0a1, "sus4" },
            { 0x491, "7" }, { 0x891, "maj7" }, { 0x489, "m7" }, { 0x449, "m7b5" }, { 0x249, "dim7" }, { 0x291, "6" }, { 0x289, "m6" },
            { 0x495, "9" }, { 0x895, "maj9" }, { 0x48d, "m9" }, { 0x095, "add9" }, { 0x889, "m(maj7)" }, { 0x44d, "m9b5" },
            { 0x88d, "m(maj9)" }, { 0x48b, "m7b9" }, { 0x893, "maj7b9" }, { 0x8d1, "maj7#11" }, { 0x4c1, "7sus4" } };
        for (auto& t : table) if (t.m == mask) return pl::keyNames()[rootPc % 12] + t.s;
        return pl::keyNames()[rootPc % 12];
    }

    inline juce::String roman (int degree, const std::vector<int>& iv)
    {
        static const char* up[] { "I", "II", "III", "IV", "V", "VI", "VII" };
        static const char* lo[] { "i", "ii", "iii", "iv", "v", "vi", "vii" };
        const bool minor = iv.size() > 1 && iv[1] == 3, dim = minor && iv.size() > 2 && iv[2] == 6;
        juce::String r = minor ? lo[degree % 7] : up[degree % 7];
        if (dim) r << juce::String (juce::CharPointer_UTF8 ("\xc2\xb0"));
        if (iv.size() > 3) r << (iv[3] == 11 ? "maj7" : "7");
        return r;
    }
}

class ChordEngine
{
public:
    struct PadEvent { bool on; int id; int root; int degree; float vel; };

    void prepare (double sampleRate) { sr = sampleRate; reset(); }

    void reset()
    {
        numPending = 0;
        for (auto& s : src) s = Source();
        counts.fill (0);
        progStep = -1; progCounter = 0;
    }

    // UI thread: press or release a chord pad
    void pushPad (const PadEvent& e)
    {
        int s1, n1, s2, n2;
        fifo.prepareToWrite (1, s1, n1, s2, n2);
        if (n1 > 0) padQueue[(size_t) s1] = e;
        else if (n2 > 0) padQueue[(size_t) s2] = e;
        fifo.finishedWrite (n1 + n2);
    }

    // progression player (UI sets the degrees, audio thread plays one chord per bar)
    std::array<std::atomic<int>, 8> progDegrees {};
    std::atomic<int> progLength { 0 };
    std::atomic<bool> progPlaying { false };
    std::atomic<int> progNow { -1 };
    std::atomic<int> lastRoot { -1 };
    std::atomic<int> lastMask { 0 };
    std::atomic<int> padHeld { -1 };

    void process (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, const chords::Settings& st, bool passThrough, double bpm)
    {
        out.clear();
        // pads from the UI
        int s1, n1, s2, n2;
        fifo.prepareToRead (fifo.getNumReady(), s1, n1, s2, n2);
        auto handlePad = [&] (const PadEvent& e)
        {
            if (e.on) trigger (kPadBase + e.id, e.root, e.vel, 0, st, out, true, e.degree);
            else release (kPadBase + e.id, 0, out);
        };
        for (int i = 0; i < n1; ++i) handlePad (padQueue[(size_t) (s1 + i)]);
        for (int i = 0; i < n2; ++i) handlePad (padQueue[(size_t) (s2 + i)]);
        fifo.finishedRead (n1 + n2);

        // progression player
        const int len = progLength.load();
        if (progPlaying.load() && len > 0)
        {
            const int bar = juce::jmax (1, (int) (sr * 60.0 / juce::jlimit (40.0, 240.0, bpm) * 4.0));
            int pos = 0;
            while (pos < numSamples)
            {
                if (progCounter <= 0)
                {
                    release (kProgSrc, pos, out);
                    progStep = (progStep + 1) % len;
                    const int deg = juce::jlimit (0, 6, progDegrees[(size_t) progStep].load());
                    const int root = padRoot (deg, st);
                    trigger (kProgSrc, root, 0.8f, pos, st, out, true, deg);
                    progNow = progStep;
                    progCounter = bar;
                }
                const int step = std::min (numSamples - pos, progCounter);
                pos += step; progCounter -= step;
            }
        }
        else if (progStep >= 0)
        {
            release (kProgSrc, 0, out);
            progStep = -1; progCounter = 0; progNow = -1;
        }

        for (const auto m : in)
        {
            const auto msg = m.getMessage();
            const int pos = m.samplePosition;
            if (msg.isNoteOn())
            {
                const int n = msg.getNoteNumber();
                if (st.on && ! passThrough) trigger (n, n, msg.getFloatVelocity(), pos, st, out, false, -1);
                else out.addEvent (msg, pos);
            }
            else if (msg.isNoteOff())
            {
                const int n = msg.getNoteNumber();
                if (src[(size_t) n].active) release (n, pos, out);
                else out.addEvent (msg, pos);
            }
            else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            {
                for (int i = 0; i < kNumSrc; ++i) if (src[(size_t) i].active) release (i, pos, out);
                out.addEvent (msg, pos);
            }
            else out.addEvent (msg, pos);
        }

        // fire strummed notes that are due
        int w = 0;
        for (int i = 0; i < numPending; ++i)
        {
            auto p = pending[(size_t) i];
            if (p.delay < numSamples) fire (p, p.delay, out);
            else { p.delay -= numSamples; pending[(size_t) w++] = p; }
        }
        numPending = w;
    }

    static int padRoot (int degree, const chords::Settings& st)
    {
        int r = 48 + st.key + chords::scale (st.scale)[(size_t) juce::jlimit (0, 6, degree)];
        if (r > 59) r -= 12;
        return r;
    }

private:
    static constexpr int kPadBase = 128, kProgSrc = 150, kNumSrc = 160, kMaxNotes = 8, kMaxPending = 512;
    struct Source { bool active = false; int fired[kMaxNotes] {}; int numFired = 0; };
    struct Pending { int src, note, delay; float vel; };

    void trigger (int s, int root, float vel, int pos, const chords::Settings& st, juce::MidiBuffer& out, bool forceKey, int forceDegree)
    {
        if (src[(size_t) s].active) release (s, pos, out);
        auto notes = chords::forNote (root, st, forceKey, forceDegree);
        if ((int) notes.size() > kMaxNotes) notes.resize ((size_t) kMaxNotes);

        // remember the chord for the display
        {
            int r; chords::degreeOf (root, st.key, st.scale, r);
            const int rootNote = (st.mode == 1 && ! forceKey) || forceDegree >= 0 ? root : r;
            int mask = 0;
            for (int x : notes) mask |= 1 << (((x - rootNote) % 12 + 12) % 12);
            lastRoot = ((rootNote % 12) + 12) % 12; lastMask = mask;
        }

        bool up = st.dir == 1;
        if (st.dir == 2) { up = altUp; altUp = ! altUp; }
        if (st.dir == 3) { for (size_t i = notes.size(); i > 1; --i) std::swap (notes[i - 1], notes[(size_t) rng.nextInt ((int) i)]); }
        else if (up) std::reverse (notes.begin(), notes.end());

        src[(size_t) s].active = true; src[(size_t) s].numFired = 0;
        const double strum = st.strumMs * 0.001 * sr;
        for (size_t i = 0; i < notes.size(); ++i)
        {
            const float jitter = st.human * (rng.nextFloat() - 0.5f);
            int d = pos + (int) (strum * (double) i + jitter * 0.03 * sr);
            d = juce::jmax (pos, d);
            const float v = juce::jlimit (0.05f, 1.f, vel * (1.f - st.human * 0.35f * rng.nextFloat()) * (strum > 0 ? 1.f - 0.03f * (float) i : 1.f));
            if (numPending < kMaxPending) pending[(size_t) numPending++] = { s, notes[i], d, v };
        }
    }

    void fire (const Pending& p, int pos, juce::MidiBuffer& out)
    {
        auto& s = src[(size_t) p.src];
        if (! s.active || s.numFired >= kMaxNotes) return;
        if (counts[(size_t) p.note] > 0) out.addEvent (juce::MidiMessage::noteOff (1, p.note), pos);
        out.addEvent (juce::MidiMessage::noteOn (1, p.note, p.vel), pos);
        counts[(size_t) p.note]++;
        s.fired[s.numFired++] = p.note;
    }

    void release (int s, int pos, juce::MidiBuffer& out)
    {
        // drop strummed notes that have not sounded yet
        int w = 0;
        for (int i = 0; i < numPending; ++i)
            if (pending[(size_t) i].src != s) pending[(size_t) w++] = pending[(size_t) i];
        numPending = w;
        auto& so = src[(size_t) s];
        for (int i = 0; i < so.numFired; ++i)
        {
            const int n = so.fired[i];
            if (counts[(size_t) n] > 0 && --counts[(size_t) n] == 0) out.addEvent (juce::MidiMessage::noteOff (1, n), pos);
        }
        so = Source();
    }

    double sr = 48000.0;
    std::array<Source, kNumSrc> src {};
    std::array<Pending, kMaxPending> pending {};
    int numPending = 0;
    std::array<int, 128> counts {};
    bool altUp = false;
    int progStep = -1, progCounter = 0;
    juce::Random rng;
    juce::AbstractFifo fifo { 64 };
    std::array<PadEvent, 64> padQueue {};
};
