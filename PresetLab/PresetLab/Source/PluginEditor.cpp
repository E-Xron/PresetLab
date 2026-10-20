#include "PluginEditor.h"

namespace { constexpr int kW = 1280, kH = 800; }

MainView::MainView (PresetLabProcessor& p)
    : proc (p), browser (p), chain (p), source (p), ampPage (p), cabPage (p), chordPanel (p),
      keys (p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible (browser);
    addAndMakeVisible (chain);
    addAndMakeVisible (source);
    for (int s = 0; s < pl::kNumSlots; ++s) addChildComponent (fxPages.add (new FxPage (p, s)));
    addChildComponent (ampPage);
    addChildComponent (cabPage);
    addAndMakeVisible (chordPanel);
    addAndMakeVisible (keys);
    chain.onSelect = [this] (int b) { selectBlock (b); };
    browser.onLoaded = [this] { shownPreset.clear(); };

    keys.setAvailableRange (24, 108);
    keys.setLowestVisibleKey (36);
    keys.setKeyWidth (22.f);
    keys.setScrollButtonsVisible (true);
    keys.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xffe9ebf0));
    keys.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff1b1e25));
    keys.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xffb7bcc7));
    keys.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, Theme::pink.withAlpha (0.85f));
    keys.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, Theme::violet.withAlpha (0.3f));
    keys.setColour (juce::MidiKeyboardComponent::shadowColourId, juce::Colours::black.withAlpha (0.4f));
    keys.setColour (juce::MidiKeyboardComponent::upDownButtonBackgroundColourId, Theme::panel2);
    keys.setColour (juce::MidiKeyboardComponent::upDownButtonArrowColourId, Theme::dim);
    keys.setColour (juce::MidiKeyboardComponent::textLabelColourId, juce::Colour (0xff7a8090));

    prev.setButtonText ("<"); next.setButtonText (">");
    prev.onClick = [this] { browser.step (-1); };
    next.onClick = [this] { browser.step (1); };
    presetButton.setTooltip ("Click for every preset by category");
    presetButton.onClick = [this]
    {
        juce::PopupMenu m;
        const auto& list = presets::factory();
        for (auto* cat : presets::categories())
        {
            juce::PopupMenu sub;
            for (int i = 0; i < (int) list.size(); ++i)
                if (juce::String (list[(size_t) i].category) == cat) sub.addItem (i + 1, list[(size_t) i].name, true, i == proc.presetIndex);
            m.addSubMenu (cat, sub);
        }
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetButton), [this] (int r)
        {
            if (r > 0) { proc.loadFactoryPreset (r - 1); browser.syncSelection(); shownPreset.clear(); }
        });
    };
    save.setButtonText ("Save");
    save.onClick = [this] { browser.askSave(); };
    undoB.setButtonText ("Undo"); redoB.setButtonText ("Redo");
    undoB.onClick = [this] { proc.undo.undo(); };
    redoB.onClick = [this] { proc.undo.redo(); };
    for (auto* b : { &prev, &next, &presetButton, &save, &undoB, &redoB }) addAndMakeVisible (b);
    prev.setConnectedEdges (juce::Button::ConnectedOnRight);
    presetButton.setConnectedEdges (juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
    next.setConnectedEdges (juce::Button::ConnectedOnLeft);

    const char* modeText[] { "Instrument", "Live Input", "Both" };
    const char* modeTips[] { "Play the instrument with MIDI or the keyboard", "Process your guitar, bass or mic through the chain", "Instrument and live input together" };
    for (int i = 0; i < 3; ++i)
    {
        modeB[i].setButtonText (modeText[i]);
        modeB[i].setTooltip (modeTips[i]);
        modeB[i].setColour (juce::TextButton::buttonOnColourId, Theme::violet);
        modeB[i].setConnectedEdges ((i > 0 ? juce::Button::ConnectedOnLeft : 0) | (i < 2 ? juce::Button::ConnectedOnRight : 0));
        modeB[i].onClick = [this, i] { setMode (i); };
        addAndMakeVisible (modeB[i]);
        modeB[i].setVisible (proc.hasLiveInput());
    }

    master.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    master.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    master.setRotaryParameters (Theme::kPi * 1.25f, Theme::kPi * 2.75f, true);
    master.setColour (juce::Slider::rotarySliderFillColourId, Theme::pink);
    master.setPopupDisplayEnabled (true, true, this);
    master.setTooltip ("Master volume");
    addAndMakeVisible (master);
    masterAtt = std::make_unique<APVTS::SliderAttachment> (p.apvts, "master", master);
    master.setDoubleClickReturnValue (true, -3.0);
    outL.get = [this] { return proc.outPeak[0].load(); };
    outR.get = [this] { return proc.outPeak[1].load(); };
    addAndMakeVisible (outL); addAndMakeVisible (outR);

    const int page = (int) proc.uiState.getProperty ("page", 0);
    chain.selected = juce::jlimit (0, 8, page);
    selectBlock (chain.selected);
}

MainView::~MainView() = default;

void MainView::setMode (int m)
{
    proc.undo.beginNewTransaction();
    proc.setParam ("mode", (float) m);
    selectBlock (0);
    source.showLive (m == pl::ModeLive);
}

void MainView::selectBlock (int b)
{
    chain.selected = b;
    proc.uiState.setProperty ("page", b, nullptr);
    source.setVisible (b == 0);
    for (int s = 0; s < pl::kNumSlots; ++s) fxPages[s]->setVisible (b != 0 && b != 4 && b != 5 && ChainStrip::slotOf (b) == s);
    ampPage.setVisible (b == 4);
    cabPage.setVisible (b == 5);
    chain.repaint();
    tick();
}

void MainView::tick()
{
    chain.refresh();
    if (source.isVisible()) { source.update(); source.tick(); }
    for (auto* f : fxPages) if (f->isVisible()) f->update();
    if (ampPage.isVisible()) ampPage.update();
    if (cabPage.isVisible()) cabPage.update();
    chordPanel.update();
    outL.update(); outR.update();

    const juce::String pn = proc.presetName;
    const juce::String label = (proc.presetCategory.isNotEmpty() ? proc.presetCategory + juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  ")) : juce::String()) + pn;
    if (label != shownPreset) { shownPreset = label; presetButton.setButtonText (label); browser.syncSelection(); }
    undoB.setEnabled (proc.undo.canUndo()); redoB.setEnabled (proc.undo.canRedo());
    const int mode = (int) proc.pv ("mode");
    if (mode != lastMode)
    {
        lastMode = mode;
        for (int i = 0; i < 3; ++i) modeB[i].setToggleState (i == mode, juce::dontSendNotification);
    }
}

void MainView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::bg);
    // top bar
    auto top = juce::Rectangle<float> (0.f, 0.f, (float) kW, 60.f);
    g.setGradientFill (juce::ColourGradient (Theme::panel, 0.f, 0.f, Theme::bg, 0.f, 60.f, false));
    g.fillRect (top);
    g.setColour (Theme::border);
    g.drawHorizontalLine (60, 0.f, (float) kW);
    drawLogoIcon (g, { 16.f, 12.f, 36.f, 36.f });
    drawWordmark (g, { 60.f, 10.f, 190.f, 40.f }, 23.f);
    g.setColour (Theme::faint); g.setFont (Theme::font (10.f, true));
    g.drawText (proc.isFxBuild() ? "FX" : "", 175, 22, 40, 16, juce::Justification::centredLeft);
    g.setColour (Theme::dim); g.setFont (Theme::font (10.5f, true));
    g.drawText ("MASTER", 1132, 22, 50, 16, juce::Justification::centredRight);
}

void MainView::resized()
{
    prev.setBounds (262, 14, 32, 32);
    presetButton.setBounds (294, 14, 300, 32);
    next.setBounds (594, 14, 32, 32);
    save.setBounds (634, 14, 64, 32);
    undoB.setBounds (712, 14, 60, 32);
    redoB.setBounds (776, 14, 60, 32);
    for (int i = 0; i < 3; ++i) modeB[i].setBounds (862 + i * 88, 14, 88, 32);
    master.setBounds (1186, 10, 40, 40);
    outL.setBounds (1238, 12, 6, 36); outR.setBounds (1247, 12, 6, 36);

    browser.setBounds (10, 68, 240, 722);
    chain.setBounds (258, 68, 1012, 104);
    const juce::Rectangle<int> page (258, 180, 1012, 380);
    source.setBounds (page);
    for (auto* f : fxPages) f->setBounds (page);
    ampPage.setBounds (page);
    cabPage.setBounds (page);
    chordPanel.setBounds (258, 568, 1012, 150);
    keys.setBounds (258, 724, 1012, 66);
    keys.setKeyWidth (1012.f / 36.f * 0.98f);
}

// ---------------------------------------------------------------------------------------------------------------------

PresetLabEditor::PresetLabEditor (PresetLabProcessor& p)
    : AudioProcessorEditor (p), proc (p), view (p)
{
    setLookAndFeel (&laf);
    juce::LookAndFeel::setDefaultLookAndFeel (&laf);
    addAndMakeVisible (view);
    proc.onPresetChanged = [this] { presetChanged = true; };
    setResizable (true, true);
    setResizeLimits (kW * 3 / 4, kH * 3 / 4, kW * 2, kH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kW / kH);
    const int w = (int) proc.uiState.getProperty ("width", kW);
    setSize (juce::jlimit (kW * 3 / 4, kW * 2, w), juce::jlimit (kH * 3 / 4, kH * 2, w * kH / kW));
    startTimerHz (30);
}

PresetLabEditor::~PresetLabEditor()
{
    stopTimer();
    proc.onPresetChanged = nullptr;
    juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void PresetLabEditor::paint (juce::Graphics& g) { g.fillAll (Theme::bg); }

void PresetLabEditor::resized()
{
    view.setBounds (0, 0, kW, kH);
    view.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kW));
    proc.uiState.setProperty ("width", getWidth(), nullptr);
}

void PresetLabEditor::timerCallback()
{
    if (presetChanged.exchange (false)) view.selectBlock (view.selectedBlock());
    view.tick();
}
