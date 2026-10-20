#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Every parameter, its range and the names shown in the UI.
namespace pl
{
    // ---------------- instruments
    enum Inst { AcousticGtr, ElectricGtr, BassGtr, SynthBass, Piano, EPiano, Organ, Synth, Pad, Strings, Brass, Bell, Mallets, Flute, Drums, kNumInst };

    inline const juce::StringArray& instNames()
    {
        static juce::StringArray n { "Acoustic Guitar", "Electric Guitar", "Bass Guitar", "Synth Bass", "Grand Piano", "Electric Piano", "Organ",
                                     "Synth", "Pad", "Strings", "Brass", "Bell / Pluck", "Mallets", "Flute / Wind", "Drum Kit" };
        return n;
    }
    inline const char* instShort (int i)
    {
        static const char* n[] { "Acoustic", "Electric", "Bass", "Synth Bass", "Piano", "E-Piano", "Organ", "Synth", "Pad", "Strings", "Brass", "Bell", "Mallets", "Wind", "Drums" };
        return n[juce::jlimit (0, kNumInst - 1, i)];
    }
    // what the two "character" knobs do for each instrument
    inline juce::String charALabel (int i)
    {
        static const char* n[] { "Pick Pos", "Pickup", "Pluck", "Shape", "Hardness", "Bell", "Drawbars", "Shape", "Shimmer", "Ensemble", "Bite", "Metal", "Material", "Breath", "Tune" };
        return n[juce::jlimit (0, kNumInst - 1, i)];
    }
    inline juce::String charBLabel (int i)
    {
        static const char* n[] { "Body", "Bite", "Thump", "Sub", "Resonance", "Bark", "Percussion", "Sub", "Motion", "Bow", "Breath", "Shine", "Resonator", "Overblow", "Snap" };
        return n[juce::jlimit (0, kNumInst - 1, i)];
    }
    // plucked and struck instruments decay on their own: Decay sets how long, Sustain is not used
    inline bool isStruck (int i) { return i == AcousticGtr || i == ElectricGtr || i == BassGtr || i == Piano || i == EPiano || i == Bell || i == Mallets || i == Drums; }
    inline bool usesWave (int i) { return i == SynthBass || i == Synth || i == Pad || i == Brass; }

    inline const juce::StringArray& kitNames()  { static juce::StringArray n { "Studio Kit", "Rock Kit", "808", "909", "Lo-Fi Kit", "Electro Kit" }; return n; }
    inline const juce::StringArray& waveNames() { static juce::StringArray n { "Saw", "Square", "Triangle", "Sine", "Supersaw", "Pulse" }; return n; }

    // ---------------- effects (stomp and rack slots)
    constexpr int kNumSlots = 6;               // 1-3 stomps (before the amp), 4-6 rack (after the cab)
    enum Fx { Overdrive, Distortion, Fuzz, Compressor, NoiseGate, Chorus, Flanger, Phaser, Tremolo, AutoWah, Eq3, Delay, PingPong, TapeEcho,
              RoomVerb, HallVerb, PlateVerb, SpringVerb, Rotary, AutoFilter, BitCrusher, Widener, LoFi, kNumFx };

    struct FxInfo { const char* name; const char* knobs[4]; float def[4]; float mix; juce::uint32 colour; };
    inline const FxInfo& fxInfo (int t)
    {
        static const FxInfo list[kNumFx] {
            { "Overdrive",      { "Drive", "Tone", "Level", nullptr },        { 0.5f, 0.5f, 0.5f, 0.5f }, 1.0f, 0xff35c76a },
            { "Distortion",     { "Drive", "Tone", "Level", nullptr },        { 0.6f, 0.5f, 0.5f, 0.5f }, 1.0f, 0xffff8a2b },
            { "Fuzz",           { "Fuzz", "Tone", "Level", "Gate" },          { 0.7f, 0.5f, 0.5f, 0.2f }, 1.0f, 0xffff4757 },
            { "Compressor",     { "Sustain", "Attack", "Level", nullptr },    { 0.5f, 0.3f, 0.5f, 0.5f }, 1.0f, 0xff4d8dff },
            { "Noise Gate",     { "Threshold", "Release", nullptr, nullptr }, { 0.35f, 0.3f, 0.5f, 0.5f }, 1.0f, 0xff9aa3b2 },
            { "Chorus",         { "Rate", "Depth", "Tone", nullptr },         { 0.3f, 0.5f, 0.6f, 0.5f }, 0.5f, 0xff38d6c4 },
            { "Flanger",        { "Rate", "Depth", "Feedback", "Manual" },    { 0.2f, 0.6f, 0.5f, 0.4f }, 0.5f, 0xffb06cff },
            { "Phaser",         { "Rate", "Depth", "Feedback", nullptr },     { 0.3f, 0.7f, 0.4f, 0.5f }, 0.5f, 0xffff5fc8 },
            { "Tremolo",        { "Rate", "Depth", "Shape", nullptr },        { 0.45f, 0.6f, 0.2f, 0.5f }, 1.0f, 0xffffd23d },
            { "Auto-Wah",       { "Sensitivity", "Range", "Resonance", nullptr }, { 0.5f, 0.6f, 0.6f, 0.5f }, 1.0f, 0xffa6e22e },
            { "3-Band EQ",      { "Low", "Mid", "High", "Mid Freq" },         { 0.5f, 0.5f, 0.5f, 0.5f }, 1.0f, 0xffd8dde6 },
            { "Delay",          { "Time", "Feedback", "Tone", nullptr },      { 0.35f, 0.35f, 0.6f, 0.5f }, 0.3f, 0xff2ec4b6 },
            { "Ping-Pong",      { "Time", "Feedback", "Tone", nullptr },      { 0.3f, 0.4f, 0.6f, 0.5f }, 0.3f, 0xff1fa2ff },
            { "Tape Echo",      { "Time", "Feedback", "Wobble", "Tone" },     { 0.35f, 0.4f, 0.3f, 0.4f }, 0.3f, 0xffd9a066 },
            { "Room Reverb",    { "Size", "Damping", "Pre-Delay", nullptr },  { 0.5f, 0.5f, 0.1f, 0.5f }, 0.25f, 0xff7c6cff },
            { "Hall Reverb",    { "Size", "Damping", "Pre-Delay", nullptr },  { 0.6f, 0.4f, 0.25f, 0.5f }, 0.3f, 0xff9b7bff },
            { "Plate Reverb",   { "Size", "Damping", "Pre-Delay", nullptr },  { 0.5f, 0.2f, 0.1f, 0.5f }, 0.3f, 0xffc77dff },
            { "Spring Reverb",  { "Decay", "Tone", "Drip", nullptr },         { 0.5f, 0.5f, 0.5f, 0.5f }, 0.3f, 0xff6f87a6 },
            { "Rotary Speaker", { "Speed", "Drive", "Horn/Drum", nullptr },   { 0.0f, 0.2f, 0.5f, 0.5f }, 1.0f, 0xffc0884a },
            { "Auto Filter",    { "Rate", "Depth", "Cutoff", "Resonance" },   { 0.3f, 0.6f, 0.4f, 0.5f }, 1.0f, 0xffff6b6b },
            { "Bit Crusher",    { "Bits", "Rate", "Tone", nullptr },          { 0.5f, 0.3f, 0.6f, 0.5f }, 1.0f, 0xff8fd14f },
            { "Stereo Widener", { "Width", "Haas", nullptr, nullptr },        { 0.6f, 0.2f, 0.5f, 0.5f }, 1.0f, 0xff5ad1ff },
            { "Lo-Fi Tape",     { "Wow", "Noise", "Age", nullptr },           { 0.3f, 0.2f, 0.5f, 0.5f }, 1.0f, 0xffbfa27a },
        };
        return list[juce::jlimit (0, kNumFx - 1, t)];
    }
    inline juce::StringArray fxNames() { juce::StringArray n; for (int i = 0; i < kNumFx; ++i) n.add (fxInfo (i).name); return n; }
    inline juce::String slotId (int s, const char* what) { return "fx" + juce::String (s + 1) + "_" + what; }
    inline const char* knobKey (int k) { static const char* n[] { "a", "b", "c", "d" }; return n[k]; }

    // ---------------- amp and cab
    inline const juce::StringArray& ampNames()
    {
        static juce::StringArray n { "Clean American", "Jazz Clean", "British Crunch", "Hot Rod Lead", "Modern Metal", "Bass Tube", "Bass Modern", "Keys Hi-Fi", "Lo-Fi Practice" };
        return n;
    }
    inline const juce::StringArray& cabNames()
    {
        static juce::StringArray n { "1x12 Open Back", "2x12 Blue", "4x12 British", "4x12 Modern", "Tweed Combo", "1x15 Bass", "4x10 Bass", "8x10 Bass", "Full-Range Keys", "Vintage Radio" };
        return n;
    }
    inline const char* ampKnob (int i) { static const char* n[] { "Gain", "Bass", "Middle", "Treble", "Presence", "Master" }; return n[i]; }
    inline const char* ampId (int i)   { static const char* n[] { "amp_gain", "amp_bass", "amp_mid", "amp_treble", "amp_presence", "amp_master" }; return n[i]; }

    // ---------------- chords
    inline const juce::StringArray& keyNames()   { static juce::StringArray n { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" }; return n; }
    inline const juce::StringArray& scaleNames() { static juce::StringArray n { "Major", "Minor", "Harmonic Minor", "Dorian", "Mixolydian", "Lydian", "Phrygian" }; return n; }
    inline const juce::StringArray& chordTypeNames()
    {
        static juce::StringArray n { "Major", "Minor", "Power", "Sus2", "Sus4", "7", "Maj7", "m7", "6", "m6", "9", "Maj9", "m9", "Add9", "Dim", "Aug" };
        return n;
    }
    inline const juce::StringArray& extNames()     { static juce::StringArray n { "Triads", "7ths", "9ths", "Power" }; return n; }
    inline const juce::StringArray& voicingNames() { static juce::StringArray n { "Close", "Open", "Spread", "Drop 2", "Guitar" }; return n; }
    inline const juce::StringArray& invNames()     { static juce::StringArray n { "Root", "1st Inv", "2nd Inv", "3rd Inv" }; return n; }
    inline const juce::StringArray& strumNames()   { static juce::StringArray n { "Down", "Up", "Alternate", "Random" }; return n; }
    inline const juce::StringArray& modeNames()    { static juce::StringArray n { "Instrument", "Live Input", "Both" }; return n; }
    inline const juce::StringArray& inChNames()    { static juce::StringArray n { "Input 1 (L)", "Input 2 (R)", "Stereo" }; return n; }

    enum Mode { ModeInst, ModeLive, ModeBoth };

    inline juce::NormalisableRange<float> logRange (float lo, float hi)
    {
        return { lo, hi,
                 [] (float s, float e, float v) { return s * std::pow (e / s, v); },
                 [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); },
                 [] (float s, float e, float v) { return juce::jlimit (s, e, v); } };
    }

    inline juce::String secText (float v, int)
    {
        return v < 1.f ? juce::String (juce::roundToInt (v * 1000.f)) + " ms" : juce::String (v, 2) + " s";
    }
    inline juce::String hzText (float v, int)
    {
        return v >= 1000.f ? juce::String (v / 1000.f, v >= 10000.f ? 1 : 2) + " kHz" : juce::String (juce::roundToInt (v)) + " Hz";
    }
    inline juce::String pctText (float v, int) { return juce::String (juce::roundToInt (v * 100.f)) + "%"; }
    inline juce::String tenText (float v, int) { return juce::String (v * 10.f, 1); }
    inline juce::String dbText  (float v, int) { return (v > 0.f ? "+" : "") + juce::String (v, 1) + " dB"; }

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout (bool fxBuild)
    {
        using namespace juce;
        std::vector<std::unique_ptr<RangedAudioParameter>> p;
        auto fl = [&] (const String& id, const String& name, NormalisableRange<float> r, float def,
                       std::function<String (float, int)> txt = nullptr, const String& unit = {})
        {
            auto attr = AudioParameterFloatAttributes().withLabel (unit);
            if (txt) attr = attr.withStringFromValueFunction (txt);
            p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def, attr));
        };
        auto ch = [&] (const String& id, const String& name, const StringArray& c, int def)
        { p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, c, def)); };
        auto bo = [&] (const String& id, const String& name, bool def)
        { p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
        auto in = [&] (const String& id, const String& name, int lo, int hi, int def)
        { p.push_back (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def)); };
        const NormalisableRange<float> u (0.f, 1.f);

        // global
        ch ("mode", "Mode", modeNames(), fxBuild ? ModeLive : ModeInst);
        ch ("inCh", "Input Channel", inChNames(), 0);
        fl ("inGain", "Input Gain", { -24.f, 24.f, 0.1f }, 0.f, dbText);
        fl ("master", "Master Volume", { -48.f, 12.f, 0.1f, 2.5f }, -3.f, dbText);

        // instrument
        ch ("inst", "Instrument", instNames(), Piano);
        ch ("kit", "Drum Kit", kitNames(), 0);
        ch ("wave", "Waveform", waveNames(), 0);
        fl ("tone", "Tone", u, 0.6f, pctText);
        fl ("charA", "Character A", u, 0.5f, pctText);
        fl ("charB", "Character B", u, 0.5f, pctText);
        fl ("atk", "Attack", logRange (0.001f, 5.f), 0.002f, secText);
        fl ("dec", "Decay", logRange (0.02f, 10.f), 1.5f, secText);
        fl ("sus", "Sustain", u, 0.8f, pctText);
        fl ("rel", "Release", logRange (0.01f, 10.f), 0.35f, secText);
        fl ("cutoff", "Filter Cutoff", logRange (40.f, 20000.f), 20000.f, hzText);
        fl ("reso", "Resonance", u, 0.1f, pctText);
        fl ("fenv", "Filter Envelope", { -1.f, 1.f }, 0.f, pctText);
        fl ("detune", "Detune", u, 0.2f, pctText);
        fl ("vibrato", "Vibrato", u, 0.f, pctText);
        fl ("width", "Stereo Width", u, 0.5f, pctText);
        in ("octave", "Octave", -3, 3, 0);
        fl ("fine", "Fine Tune", { -100.f, 100.f, 1.f }, 0.f, [] (float v, int) { return String (roundToInt (v)) + " ct"; });
        fl ("velSens", "Velocity", u, 0.7f, pctText);
        fl ("instVol", "Instrument Level", { -36.f, 12.f, 0.1f, 2.f }, 0.f, dbText);

        // stomps + rack
        for (int s = 0; s < kNumSlots; ++s)
        {
            const String n = (s < 3 ? "Stomp " + String (s + 1) : "Rack " + String (s - 2)) + " ";
            bo (slotId (s, "on"), n + "On", false);
            ch (slotId (s, "type"), n + "Type", fxNames(), s == 0 ? Overdrive : s == 1 ? Chorus : s == 2 ? Delay : s == 3 ? Compressor : s == 4 ? Delay : HallVerb);
            for (int k = 0; k < 4; ++k)
                fl (slotId (s, knobKey (k)), n + "Knob " + String (k + 1), u, 0.5f, pctText);
            fl (slotId (s, "mix"), n + "Mix", u, 0.5f, pctText);
        }

        // amp + cab
        bo ("amp_on", "Amp On", false);
        ch ("amp_model", "Amp Model", ampNames(), 0);
        const float ampDef[6] { 0.4f, 0.5f, 0.5f, 0.5f, 0.5f, 0.6f };
        for (int i = 0; i < 6; ++i) fl (ampId (i), String ("Amp ") + ampKnob (i), u, ampDef[i], tenText);
        bo ("cab_on", "Cab On", false);
        ch ("cab_model", "Cab Model", cabNames(), 2);
        fl ("cab_mic", "Mic Position", u, 0.35f, pctText);
        fl ("cab_room", "Room", u, 0.15f, pctText);
        fl ("cab_level", "Cab Level", { -18.f, 12.f, 0.1f }, 0.f, dbText);

        // chord generator
        bo ("ch_on", "Chords On", false);
        ch ("ch_mode", "Chord Mode", StringArray { "Follow Key", "Fixed Chord" }, 0);
        ch ("ch_key", "Key", keyNames(), 0);
        ch ("ch_scale", "Scale", scaleNames(), 0);
        ch ("ch_type", "Chord Type", chordTypeNames(), 0);
        ch ("ch_ext", "Chord Size", extNames(), 0);
        ch ("ch_voicing", "Voicing", voicingNames(), 0);
        ch ("ch_inv", "Inversion", invNames(), 0);
        fl ("ch_strum", "Strum", { 0.f, 250.f, 1.f }, 0.f, [] (float v, int) { return String (roundToInt (v)) + " ms"; });
        ch ("ch_dir", "Strum Direction", strumNames(), 0);
        bo ("ch_bass", "Bass Note", false);
        fl ("ch_human", "Humanize", u, 0.f, pctText);

        return { p.begin(), p.end() };
    }
}
