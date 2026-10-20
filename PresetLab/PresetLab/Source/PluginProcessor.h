#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>
#include "Params.h"
#include "Instrument.h"
#include "Effects.h"
#include "ChordEngine.h"
#include "Presets.h"

#ifndef PRESETLAB_FX
 #define PRESETLAB_FX 0
#endif

class PresetLabProcessor : public juce::AudioProcessor
{
public:
    PresetLabProcessor();
    ~PresetLabProcessor() override = default;

    void prepareToPlay (double sampleRate, int blockSize) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return PRESETLAB_FX ? "PresetLab FX" : "PresetLab"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboard;
    ChordEngine chordEngine;

    // presets (message thread)
    void loadFactoryPreset (int index);
    void loadUserPreset (const juce::File&);
    void saveUserPreset (const juce::String& name);
    static juce::File userPresetFolder();
    juce::String presetName { "Init" };
    juce::String presetCategory;
    int presetIndex = -1;                       // factory index, -1 for user / edited
    std::function<void()> onPresetChanged;

    float pv (const juce::String& id) const { return apvts.getRawParameterValue (id)->load(); }
    void setParam (const juce::String& id, float plainValue);
    chords::Settings chordSettings() const;
    bool hasLiveInput() const { return getTotalNumInputChannels() > 0; }
    bool isFxBuild() const { return PRESETLAB_FX != 0; }

    // meters + tuner
    std::atomic<float> inPeak { 0.f }, outPeak[2] { { 0.f }, { 0.f } };
    float detectPitch();
    double getSr() const { return sr; }
    std::array<std::atomic<float>, pl::kNumSlots> fxMeter {};
    juce::ValueTree uiState { "UI" };

private:
    static BusesProperties makeBuses();
    void readSettings();
    void applyTree (const juce::ValueTree&);

    PlSynth synth;
    SoundSettings settings;
    std::array<FxUnit, pl::kNumSlots> fx;
    AmpSim amp;
    CabSim cab;
    juce::MidiBuffer chordMidi;
    juce::AudioBuffer<float> work;
    juce::SmoothedValue<float> masterSm, instSm, inSm;
    double sr = 48000.0;

    std::vector<float> tunerRing;
    std::atomic<int> tunerPos { 0 };

    struct SlotPtrs { std::atomic<float>* on, * type, * k[4], * mix; };
    std::array<SlotPtrs, pl::kNumSlots> sp {};
    std::atomic<float>* p[64] {};
    enum PIdx { iMode, iInCh, iInGain, iMaster, iInst, iKit, iWave, iTone, iChA, iChB, iAtk, iDec, iSus, iRel, iCut, iRes, iFenv, iDet, iVib, iWidth,
                iOct, iFine, iVel, iInstVol, iAmpOn, iAmpModel, iAmpK0, iCabOn = iAmpK0 + 6, iCabModel, iCabMic, iCabRoom, iCabLevel, kNumP };
};
