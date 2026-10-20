#pragma once
#include "PluginProcessor.h"
#include "Panels.h"

// Everything is laid out on a 1280 x 800 canvas that is scaled to the window size.
class MainView : public juce::Component
{
public:
    explicit MainView (PresetLabProcessor&);
    ~MainView() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
    void selectBlock (int b);
    int selectedBlock() const { return chain.selected; }
    void showSourceTab (bool live) { source.showLive (live); }

private:
    void setMode (int m);

    PresetLabProcessor& proc;
    Browser browser;
    ChainStrip chain;
    SourcePage source;
    juce::OwnedArray<FxPage> fxPages;
    AmpPage ampPage;
    CabPage cabPage;
    ChordPanel chordPanel;
    juce::MidiKeyboardComponent keys;

    juce::TextButton prev, next, presetButton, save, undoB, redoB, modeB[3];
    juce::Slider master;
    std::unique_ptr<APVTS::SliderAttachment> masterAtt;
    Meter outL, outR, inMeter;
    juce::String shownPreset;
    int lastMode = -1;
};

class PresetLabEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PresetLabEditor (PresetLabProcessor&);
    ~PresetLabEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    MainView& getView() { return view; }

private:
    void timerCallback() override;

    PresetLabProcessor& proc;
    PLLookAndFeel laf;
    MainView view;
    juce::TooltipWindow tooltips { this, 600 };
    std::atomic<bool> presetChanged { true };
};
