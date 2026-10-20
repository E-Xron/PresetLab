#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Tuner.h"

namespace
{
    // parameters that presets never touch (they belong to the performer, not the sound)
    bool isGlobal (const juce::String& id)
    {
        return id == "mode" || id == "inCh" || id == "inGain" || id == "master" || id.startsWith ("ch_");
    }

    // safety limiter: transparent below -2 dBFS, soft above
    inline float safety (float x)
    {
        const float a = std::abs (x);
        if (a < 0.8f) return x;
        const float y = 0.8f + 0.2f * dsp1::fastTanh ((a - 0.8f) / 0.2f);
        return x < 0 ? -y : y;
    }
}

PresetLabProcessor::BusesProperties PresetLabProcessor::makeBuses()
{
    BusesProperties b;
   #if PRESETLAB_FX
    b = b.withInput ("Input", juce::AudioChannelSet::stereo(), true);
   #else
    // the instrument gets a live input only as a standalone app (DAWs use PresetLab FX for that)
    if (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_Standalone)
        b = b.withInput ("Input", juce::AudioChannelSet::stereo(), true);
   #endif
    return b.withOutput ("Output", juce::AudioChannelSet::stereo(), true);
}

PresetLabProcessor::PresetLabProcessor()
    : AudioProcessor (makeBuses()),
      apvts (*this, &undo, "STATE", pl::createLayout (PRESETLAB_FX != 0))
{
    const char* ids[] { "mode", "inCh", "inGain", "master", "inst", "kit", "wave", "tone", "charA", "charB", "atk", "dec", "sus", "rel", "cutoff", "reso",
                        "fenv", "detune", "vibrato", "width", "octave", "fine", "velSens", "instVol", "amp_on", "amp_model",
                        "amp_gain", "amp_bass", "amp_mid", "amp_treble", "amp_presence", "amp_master",
                        "cab_on", "cab_model", "cab_mic", "cab_room", "cab_level" };
    static_assert (sizeof (ids) / sizeof (ids[0]) == kNumP, "parameter table out of sync");
    for (int i = 0; i < kNumP; ++i) p[i] = apvts.getRawParameterValue (ids[i]);
    for (int s = 0; s < pl::kNumSlots; ++s)
    {
        auto g = [&] (const char* w) { return apvts.getRawParameterValue (pl::slotId (s, w)); };
        sp[(size_t) s] = { g ("on"), g ("type"), { g ("a"), g ("b"), g ("c"), g ("d") }, g ("mix") };
    }

    synth.settings = &settings;
    for (int i = 0; i < 24; ++i) synth.addVoice (new PlVoice (settings, i));
    synth.addSound (new PlSound());
    synth.setNoteStealingEnabled (true);
    tunerRing.assign (4096, 0.f);

    const juce::String first = PRESETLAB_FX ? "Clean Amp" : "Concert Grand";
    const auto& list = presets::factory();
    for (size_t i = 0; i < list.size(); ++i)
        if (first == list[i].name) { loadFactoryPreset ((int) i); break; }
    undo.clearUndoHistory();
}

bool PresetLabProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    if (l.inputBuses.size() > 0)
    {
        const auto in = l.getMainInputChannelSet();
        if (! in.isDisabled() && in != juce::AudioChannelSet::stereo() && in != juce::AudioChannelSet::mono()) return false;
    }
    return true;
}

void PresetLabProcessor::prepareToPlay (double sampleRate, int blockSize)
{
    sr = sampleRate;
    synth.setCurrentPlaybackSampleRate (sr);
    chordEngine.prepare (sr);
    for (auto& f : fx) f.prepare (sr, juce::jmax (512, blockSize));
    amp.prepare (sr, 512);
    cab.prepare (sr);
    work.setSize (2, juce::jmax (512, blockSize));
    masterSm.reset (sr, 0.05); instSm.reset (sr, 0.05); inSm.reset (sr, 0.05);
    masterSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p[iMaster]->load()));
    instSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p[iInstVol]->load()));
    inSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p[iInGain]->load()));
    keyboard.reset();
}

void PresetLabProcessor::readSettings()
{
    auto& s = settings;
    s.inst = (int) p[iInst]->load(); s.kit = (int) p[iKit]->load(); s.wave = (int) p[iWave]->load();
    s.tone = p[iTone]->load(); s.charA = p[iChA]->load(); s.charB = p[iChB]->load();
    s.atk = p[iAtk]->load(); s.dec = p[iDec]->load(); s.sus = p[iSus]->load(); s.rel = p[iRel]->load();
    s.cutoff = p[iCut]->load(); s.reso = p[iRes]->load(); s.fenv = p[iFenv]->load();
    s.detune = p[iDet]->load(); s.vibrato = p[iVib]->load(); s.width = p[iWidth]->load(); s.velSens = p[iVel]->load();
    s.pitch = p[iOct]->load() * 12.f + p[iFine]->load() / 100.f;
}

chords::Settings PresetLabProcessor::chordSettings() const
{
    chords::Settings c;
    c.on = pv ("ch_on") > 0.5f; c.mode = (int) pv ("ch_mode"); c.key = (int) pv ("ch_key"); c.scale = (int) pv ("ch_scale");
    c.type = (int) pv ("ch_type"); c.ext = (int) pv ("ch_ext"); c.voicing = (int) pv ("ch_voicing"); c.inv = (int) pv ("ch_inv");
    c.strumMs = pv ("ch_strum"); c.dir = (int) pv ("ch_dir"); c.bass = pv ("ch_bass") > 0.5f; c.human = pv ("ch_human");
    return c;
}

void PresetLabProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    if (work.getNumSamples() < n) work.setSize (2, n, false, false, true);
    readSettings();
    const int mode = (int) p[iMode]->load();

    // MIDI: on-screen keyboard + chord generator
    keyboard.processNextMidiBuffer (midi, 0, n, true);
    double bpm = 110.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm()) bpm = *b;
    chordEngine.process (midi, chordMidi, n, chordSettings(), settings.inst == pl::Drums, bpm);

    // live input
    work.clear();
    auto* L = work.getWritePointer (0);
    auto* R = work.getWritePointer (1);
    float inPk = 0.f;
    if (numIn > 0 && mode != pl::ModeInst)
    {
        const int ch = (int) p[iInCh]->load();
        const float* a = buffer.getReadPointer (0);
        const float* b = numIn > 1 ? buffer.getReadPointer (1) : a;
        inSm.setTargetValue (juce::Decibels::decibelsToGain (p[iInGain]->load()));
        int tp = tunerPos.load();
        for (int i = 0; i < n; ++i)
        {
            const float g = inSm.getNextValue();
            const float l = (ch == 1 ? b[i] : a[i]) * g, r = (ch == 2 ? b[i] : ch == 1 ? b[i] : a[i]) * g;
            L[i] = l; R[i] = r;
            inPk = std::max (inPk, std::max (std::abs (l), std::abs (r)));
            tunerRing[(size_t) tp] = (l + r) * 0.5f; tp = (tp + 1) & 4095;
        }
        tunerPos = tp;
    }
    inPeak = std::max (inPk, inPeak.load() * 0.9f);

    // instrument
    for (int c = 0; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    if (mode != pl::ModeLive)
    {
        synth.renderNextBlock (buffer, chordMidi, 0, n);
        instSm.setTargetValue (juce::Decibels::decibelsToGain (p[iInstVol]->load()));
        const float* sL = buffer.getReadPointer (0);
        const float* sR = numOut > 1 ? buffer.getReadPointer (1) : sL;
        for (int i = 0; i < n; ++i) { const float g = instSm.getNextValue(); L[i] += sL[i] * g; R[i] += sR[i] * g; }
    }

    // the chain: stomps -> amp -> cab -> rack, in chunks the amp can take
    const bool ampOn = p[iAmpOn]->load() > 0.5f, cabOn = p[iCabOn]->load() > 0.5f;
    float ampK[6]; for (int i = 0; i < 6; ++i) ampK[i] = p[iAmpK0 + i]->load();
    for (int off = 0; off < n; off += 256)
    {
        const int m = std::min (256, n - off);
        float* cl = L + off; float* cr = R + off;
        auto runSlot = [&] (int s)
        {
            const auto& q = sp[(size_t) s];
            if (q.on->load() < 0.5f) { fxMeter[(size_t) s] = 0.f; return; }
            const float k[4] { q.k[0]->load(), q.k[1]->load(), q.k[2]->load(), q.k[3]->load() };
            fx[(size_t) s].process (cl, cr, m, (int) q.type->load(), k, q.mix->load());
            fxMeter[(size_t) s] = fx[(size_t) s].meter.load();
        };
        for (int s = 0; s < 3; ++s) runSlot (s);
        if (ampOn) amp.process (cl, cr, m, (int) p[iAmpModel]->load(), ampK);
        if (cabOn) cab.process (cl, cr, m, (int) p[iCabModel]->load(), p[iCabMic]->load(), p[iCabRoom]->load(), p[iCabLevel]->load());
        for (int s = 3; s < pl::kNumSlots; ++s) runSlot (s);
    }

    // master
    masterSm.setTargetValue (juce::Decibels::decibelsToGain (p[iMaster]->load(), -47.9f));
    float pk[2] {};
    auto* oL = buffer.getWritePointer (0);
    auto* oR = numOut > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        const float g = masterSm.getNextValue();
        float l = safety (L[i] * g), r = safety (R[i] * g);
        if (! std::isfinite (l)) l = 0.f;
        if (! std::isfinite (r)) r = 0.f;
        if (oR != nullptr) { oL[i] = l; oR[i] = r; } else oL[i] = 0.5f * (l + r);
        pk[0] = std::max (pk[0], std::abs (l)); pk[1] = std::max (pk[1], std::abs (r));
    }
    for (int c = 0; c < 2; ++c) outPeak[c] = std::max (pk[c], outPeak[c].load() * 0.9f);
    for (int c = numOut; c < buffer.getNumChannels(); ++c) buffer.clear (c, 0, n);
    midi.clear();
}

float PresetLabProcessor::detectPitch()
{
    std::vector<float> x (2048);
    const int pos = tunerPos.load();
    for (int i = 0; i < 2048; ++i) x[(size_t) i] = tunerRing[(size_t) ((pos - 2048 + i) & 4095)];
    return Tuner::detect (x, sr);
}

// ---------------------------------------------------------------------------------------------------------------------
// presets

void PresetLabProcessor::setParam (const juce::String& id, float plainValue)
{
    if (auto* prm = apvts.getParameter (id))
        prm->setValueNotifyingHost (prm->convertTo0to1 (plainValue));
}

void PresetLabProcessor::loadFactoryPreset (int index)
{
    const auto& list = presets::factory();
    index = juce::jlimit (0, (int) list.size() - 1, index);
    const auto& pr = list[(size_t) index];
    undo.beginNewTransaction();

    // start from defaults (except the performer's settings)
    for (auto* prm : getParameters())
        if (auto* r = dynamic_cast<juce::RangedAudioParameter*> (prm))
            if (! isGlobal (r->getParameterID()))
                r->setValueNotifyingHost (r->getDefaultValue());

    auto choiceIndex = [] (juce::RangedAudioParameter* prm, juce::String v) -> float
    {
        v = v.replaceCharacter ('_', ' ');
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (prm))
        {
            for (int i = 0; i < c->choices.size(); ++i) if (c->choices[i].equalsIgnoreCase (v)) return (float) i;
            for (int i = 0; i < c->choices.size(); ++i) if (c->choices[i].startsWithIgnoreCase (v)) return (float) i;
        }
        return v.getFloatValue();
    };
    auto setNamed = [&] (const juce::String& id, const juce::String& value)
    {
        if (auto* prm = apvts.getParameter (id))
            setParam (id, dynamic_cast<juce::AudioParameterChoice*> (prm) != nullptr ? choiceIndex (prm, value) : value.getFloatValue());
    };

    juce::StringArray tokens; tokens.addTokens (pr.data, " ", "");
    for (auto& t : tokens)
    {
        const auto key = t.upToFirstOccurrenceOf ("=", false, false), val = t.fromFirstOccurrenceOf ("=", false, false);
        if (key.isEmpty()) continue;
        juce::StringArray parts; parts.addTokens (val, ":", "");
        if (key.length() == 2 && key[0] == 's')                 // effect slot
        {
            const int s = key.getTrailingIntValue() - 1;
            if (s < 0 || s >= pl::kNumSlots) continue;
            const int type = (int) choiceIndex (apvts.getParameter (pl::slotId (s, "type")), parts[0]);
            const auto& info = pl::fxInfo (type);
            juce::StringArray kv; kv.addTokens (parts[1], ",", "");
            setParam (pl::slotId (s, "on"), 1.f);
            setParam (pl::slotId (s, "type"), (float) type);
            for (int k = 0; k < 4; ++k) setParam (pl::slotId (s, pl::knobKey (k)), k < kv.size() && kv[k].isNotEmpty() ? kv[k].getFloatValue() : info.def[k]);
            setParam (pl::slotId (s, "mix"), parts.size() > 2 ? parts[2].getFloatValue() : info.mix);
        }
        else if (key == "amp")
        {
            setParam ("amp_on", 1.f); setNamed ("amp_model", parts[0]);
            juce::StringArray kv; kv.addTokens (parts[1], ",", "");
            for (int k = 0; k < 6 && k < kv.size(); ++k) setParam (pl::ampId (k), kv[k].getFloatValue());
        }
        else if (key == "cab")
        {
            setParam ("cab_on", 1.f); setNamed ("cab_model", parts[0]);
            juce::StringArray kv; kv.addTokens (parts[1], ",", "");
            if (kv.size() > 0) setParam ("cab_mic", kv[0].getFloatValue());
            if (kv.size() > 1) setParam ("cab_room", kv[1].getFloatValue());
            if (kv.size() > 2) setParam ("cab_level", kv[2].getFloatValue());
        }
        else setNamed (key, val);
    }

    // live presets need the input; instrument presets need the instrument
    const bool live = juce::String (pr.category).startsWith ("Live");
    const int mode = (int) pv ("mode");
    if (live && mode == pl::ModeInst && hasLiveInput()) setParam ("mode", (float) pl::ModeLive);
    if (! live && mode == pl::ModeLive) setParam ("mode", (float) pl::ModeInst);

    presetName = pr.name; presetCategory = pr.category; presetIndex = index;
    if (onPresetChanged) onPresetChanged();
}

juce::File PresetLabProcessor::userPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("PresetLab Presets");
}

void PresetLabProcessor::saveUserPreset (const juce::String& name)
{
    auto folder = userPresetFolder();
    folder.createDirectory();
    auto state = apvts.copyState();
    state.setProperty ("presetName", name, nullptr);
    if (auto xml = state.createXml())
        xml->writeTo (folder.getChildFile (juce::File::createLegalFileName (name) + ".plpreset"));
    presetName = name; presetCategory = "User"; presetIndex = -1;
    if (onPresetChanged) onPresetChanged();
}

void PresetLabProcessor::applyTree (const juce::ValueTree& tree)
{
    for (int i = 0; i < tree.getNumChildren(); ++i)
    {
        const auto c = tree.getChild (i);
        const auto id = c.getProperty ("id").toString();
        if (id.isEmpty() || isGlobal (id)) continue;
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 ((float) c.getProperty ("value")));
    }
}

void PresetLabProcessor::loadUserPreset (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
    {
        undo.beginNewTransaction();
        const auto tree = juce::ValueTree::fromXml (*xml);
        applyTree (tree);
        presetName = tree.getProperty ("presetName", f.getFileNameWithoutExtension()).toString();
        presetCategory = "User"; presetIndex = -1;
        if (onPresetChanged) onPresetChanged();
    }
}

void PresetLabProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetCategory", presetCategory, nullptr);
    state.setProperty ("presetIndex", presetIndex, nullptr);
    juce::String prog;
    for (int i = 0; i < chordEngine.progLength.load(); ++i) prog << chordEngine.progDegrees[(size_t) i].load() << " ";
    state.setProperty ("progression", prog.trim(), nullptr);
    state.removeChild (state.getChildWithName ("UI"), nullptr);
    state.addChild (uiState.createCopy(), -1, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void PresetLabProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto tree = juce::ValueTree::fromXml (*xml);
        if (! tree.hasType (apvts.state.getType())) return;
        presetName = tree.getProperty ("presetName", "Init").toString();
        presetCategory = tree.getProperty ("presetCategory").toString();
        presetIndex = tree.getProperty ("presetIndex", -1);
        juce::StringArray prog; prog.addTokens (tree.getProperty ("progression").toString(), " ", "");
        chordEngine.progLength = 0;
        for (int i = 0; i < juce::jmin (8, prog.size()); ++i) chordEngine.progDegrees[(size_t) i] = prog[i].getIntValue();
        chordEngine.progLength = juce::jmin (8, prog.size());
        const auto ui = tree.getChildWithName ("UI");
        if (ui.isValid()) uiState = ui.createCopy();
        tree.removeChild (ui, nullptr);
        apvts.replaceState (tree);
        if (onPresetChanged) onPresetChanged();
    }
}

juce::AudioProcessorEditor* PresetLabProcessor::createEditor() { return new PresetLabEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PresetLabProcessor(); }
