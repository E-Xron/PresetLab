#pragma once
#include "PluginProcessor.h"
#include "Widgets.h"
#include "Tuner.h"

namespace text
{
    inline juce::String inst (int i)
    {
        static const char* d[] {
            "Plucked acoustic strings. Pick Pos moves the pick from the bridge (bright) to the neck (warm). Body adds wooden resonance.",
            "Solid-body electric guitar. Pickup goes from bridge to neck, Bite adds pick attack and grit. Plug it into an amp and cab.",
            "Electric bass. Pluck goes from fingers to pick, Thump adds low-end punch.",
            "Analog mono-style bass. Shape adds drive (or pulse width on Pulse), Sub adds a sine an octave below.",
            "Grand piano. Hardness sets how bright the hammers are, Resonance detunes the string pairs for a richer sound.",
            "Tine electric piano. Bell brings out the chime, Bark adds growl when you play hard.",
            "Tonewheel organ. Drawbars sweep from mellow flutes to full organ, Percussion adds the ping and key click.",
            "Analog synth with two detuned oscillators. Shape adds drive (or pulse width), Sub adds an octave below.",
            "Slow, lush pad. Shimmer adds a bright octave above, Motion slowly moves the filter.",
            "String section. Ensemble widens the players, Bow adds bow noise.",
            "Brass section. Bite opens the filter on every note, Breath adds air.",
            "FM bells and plucks. Metal changes the harmonic ratio (from pure to clangy), Shine adds a second shimmering layer.",
            "Marimba to vibraphone to glockenspiel as you turn Material. Resonator lengthens the ring.",
            "Breathy flutes and whistles. Breath adds air, Overblow adds harmonics.",
            "Drum kit on the General MIDI map: C1 kick, D1 snare, F#1 closed hat, A#1 open hat, C#2 crash. Tune and Snap shape every drum." };
        return d[juce::jlimit (0, pl::kNumInst - 1, i)];
    }
    inline juce::String fx (int i)
    {
        static const char* d[] {
            "Warm, mid-forward tube-style drive. Keeps your low end clean.", "Hard clipping for crunchy rock and metal tones.",
            "Thick vintage fuzz. Gate tames the hiss between notes.", "Evens out your playing and adds sustain.",
            "Silences hum and hiss when you stop playing.", "Shimmering doubled sound that widens the stereo image.",
            "Jet-plane sweep. Manual sets the base delay, Feedback makes it ring.", "Swirling six-stage phase sweep.",
            "Rhythmic volume pulsing. Shape goes from smooth to choppy.", "A wah that follows how hard you play.",
            "Shape the lows, mids and highs.", "Clean stereo echoes.", "Echoes that bounce left and right.",
            "Warm, wobbly tape delay. Push Feedback for runaway echoes.", "Small, natural room ambience.",
            "Big, lush concert hall.", "Bright, dense studio plate.", "Boingy amp-style spring tank.",
            "Rotating speaker cabinet. Turn Speed past halfway for fast.", "Sweeping resonant filter.",
            "Lo-fi digital grit.", "Makes the sound wider. Haas adds a short delay on one side.", "Worn cassette: wow, hiss and age." };
        return d[juce::jlimit (0, pl::kNumFx - 1, i)];
    }
    inline juce::String amp (int i)
    {
        static const char* d[] {
            "Sparkling clean with scooped mids. Great for funk, country and pop.", "Warm and round. Stays clean at almost any gain.",
            "Classic rock crunch with punchy mids.", "High-gain lead with singing sustain.", "Tight, scooped, very high gain.",
            "Fat tube bass amp with growl.", "Clean, punchy modern bass rig.", "Full-range clean amp for keys and synths.",
            "Small, cheap and crunchy. Instant attitude." };
        return d[juce::jlimit (0, 8, i)];
    }
    inline juce::String cab (int i)
    {
        static const char* d[] {
            "Open-back 1x12 combo: airy and bright.", "2x12 with chimey blue speakers.", "Closed 4x12 with a classic British midrange.",
            "Modern 4x12: tight lows and cutting highs.", "Small vintage tweed combo.", "Deep 1x15 bass cab.",
            "Punchy 4x10 bass cab with some bite.", "Huge 8x10 bass fridge.", "Flat, full-range speaker for keys and synths.",
            "Tiny old radio speaker." };
        return d[juce::jlimit (0, 9, i)];
    }
    inline juce::Colour ampColour (int m)
    {
        static const juce::uint32 c[] { 0xffc9d3e0, 0xffd8c7a5, 0xffcfae5e, 0xff9aa0a8, 0xff3a3f48, 0xffb9b9b9, 0xff34495e, 0xff8a6e57, 0xff7fb3a5 };
        return juce::Colour (c[juce::jlimit (0, 8, m)]);
    }
}

// ================================================================================================
class InstTiles : public juce::Component
{
public:
    explicit InstTiles (PresetLabProcessor& p) : proc (p) {}
    int selected = -1;
    bool dimmed = false;

    juce::Rectangle<float> tile (int i) const
    {
        const float gap = 6.f, w = ((float) getWidth() - gap * 4.f) / 5.f, h = ((float) getHeight() - gap * 2.f) / 3.f;
        return { (float) (i % 5) * (w + gap), (float) (i / 5) * (h + gap), w, h };
    }
    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < pl::kNumInst; ++i)
        {
            auto r = tile (i);
            const bool sel = i == selected, hov = i == hover;
            if (sel)
            {
                g.setGradientFill (juce::ColourGradient (Theme::violet.withAlpha (0.55f), r.getX(), r.getY(), Theme::pink.withAlpha (0.35f), r.getRight(), r.getBottom(), false));
                g.fillRoundedRectangle (r, 10.f);
                g.setColour (Theme::violet.brighter (0.4f));
                g.drawRoundedRectangle (r.reduced (0.5f), 10.f, 1.5f);
            }
            else
            {
                g.setColour (Theme::panel2.brighter (hov ? 0.07f : 0.f));
                g.fillRoundedRectangle (r, 10.f);
                g.setColour (Theme::border);
                g.drawRoundedRectangle (r.reduced (0.5f), 10.f, 1.f);
            }
            const float a = dimmed && ! sel ? 0.4f : 1.f;
            drawInstIcon (g, i, r.withTrimmedBottom (20.f).reduced (r.getWidth() * 0.26f, 6.f).withY (r.getY() + 8.f),
                          (sel ? juce::Colours::white : Theme::text.withAlpha (0.75f)).withMultipliedAlpha (a));
            g.setColour ((sel ? juce::Colours::white : Theme::text.withAlpha (0.85f)).withMultipliedAlpha (a));
            g.setFont (Theme::font (11.5f, true));
            g.drawText (pl::instShort (i), r.removeFromBottom (22.f), juce::Justification::centred);
        }
    }
    void mouseMove (const juce::MouseEvent& e) override { int h = -1; for (int i = 0; i < pl::kNumInst; ++i) if (tile (i).contains (e.position)) h = i; if (h != hover) { hover = h; repaint(); } }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < pl::kNumInst; ++i)
            if (tile (i).contains (e.position))
            {
                proc.undo.beginNewTransaction();
                proc.setParam ("inst", (float) i);
                if (proc.pv ("mode") == pl::ModeLive) proc.setParam ("mode", (float) pl::ModeBoth);
            }
    }
private:
    PresetLabProcessor& proc;
    int hover = -1;
};

class TunerView : public juce::Component
{
public:
    float freq = 0.f;
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        const bool has = freq > 20.f;
        float cents = 0.f; juce::String name = "-";
        if (has)
        {
            const float midi = 69.f + 12.f * std::log2 (freq / 440.f);
            const int n = juce::roundToInt (midi);
            cents = (midi - (float) n) * 100.f;
            name = juce::MidiMessage::getMidiNoteName (n, true, false, 4);
        }
        const bool inTune = has && std::abs (cents) < 4.f;
        const auto c = ! has ? Theme::faint : inTune ? Theme::green : std::abs (cents) < 15.f ? Theme::amber : Theme::red;
        // arc scale
        const auto ctr = juce::Point<float> (r.getCentreX(), r.getBottom() - 30.f);
        const float rad = std::min (r.getWidth() * 0.42f, r.getHeight() - 50.f);
        for (int i = -50; i <= 50; i += 5)
        {
            const float a = (float) i / 50.f * 1.05f;
            const float r0 = rad - (i % 25 == 0 ? 14.f : 8.f);
            g.setColour (i == 0 ? Theme::green : Theme::faint);
            g.drawLine (ctr.x + r0 * std::sin (a), ctr.y - r0 * std::cos (a), ctr.x + rad * std::sin (a), ctr.y - rad * std::cos (a), i == 0 ? 2.5f : 1.2f);
        }
        if (has)
        {
            const float a = juce::jlimit (-50.f, 50.f, cents) / 50.f * 1.05f;
            g.setColour (c);
            g.drawLine (ctr.x, ctr.y, ctr.x + (rad - 4.f) * std::sin (a), ctr.y - (rad - 4.f) * std::cos (a), 3.f);
            g.fillEllipse (juce::Rectangle<float> (12.f, 12.f).withCentre (ctr));
        }
        g.setColour (c);
        g.setFont (Theme::font (54.f, true));
        g.drawText (name, juce::Rectangle<float> (r.getX(), ctr.y - rad * 0.72f, r.getWidth(), 60.f), juce::Justification::centred);
        g.setColour (Theme::dim);
        g.setFont (Theme::font (13.f));
        g.drawText (has ? juce::String (freq, 1) + " Hz   " + (cents >= 0 ? "+" : "") + juce::String (cents, 0) + " cents" : "Play a note to tune",
                    r.removeFromBottom (20.f), juce::Justification::centred);
    }
};

// ================================================================================================
class SourcePage : public juce::Component
{
public:
    explicit SourcePage (PresetLabProcessor& p) : proc (p), tiles (p),
        wave (p.apvts, "wave"), kit (p.apvts, "kit"), inCh (p.apvts, "inCh")
    {
        auto& s = p.apvts;
        const auto v = Theme::violet;
        auto mk = [&] (const char* id, const char* name, bool bip = false) { auto* k = knobs.add (new Knob (s, id, name, v, bip)); addAndMakeVisible (k); return k; };
        tone = mk ("tone", "Tone"); chA = mk ("charA", "A"); chB = mk ("charB", "B"); level = mk ("instVol", "Level"); oct = mk ("octave", "Octave", true); fine = mk ("fine", "Fine", true);
        atk = mk ("atk", "Attack"); dec = mk ("dec", "Decay"); sus = mk ("sus", "Sustain"); rel = mk ("rel", "Release"); vel = mk ("velSens", "Velocity"); width = mk ("width", "Width");
        cut = mk ("cutoff", "Cutoff"); res = mk ("reso", "Resonance"); fenv = mk ("fenv", "Filter Env", true); det = mk ("detune", "Detune"); vib = mk ("vibrato", "Vibrato");
        addAndMakeVisible (tiles);
        addChildComponent (wave); addChildComponent (kit);
        waveLabel.setFont (Theme::font (12.f, true)); waveLabel.setColour (juce::Label::textColourId, Theme::dim);
        addChildComponent (waveLabel);

        // live input
        gain = std::make_unique<Knob> (s, "inGain", "Input Gain", Theme::cyan, true);
        addChildComponent (*gain); addChildComponent (inCh); addChildComponent (tuner); addChildComponent (meter);
        meter.horizontal = true;
        meter.get = [this] { return proc.inPeak.load(); };

        for (auto* b : { &tabInst, &tabLive })
        {
            b->setClickingTogglesState (false);
            b->setColour (juce::TextButton::buttonOnColourId, Theme::violet);
            b->getProperties().set ("radius", 13);
            addAndMakeVisible (b);
        }
        tabInst.setButtonText ("INSTRUMENT"); tabLive.setButtonText ("LIVE INPUT");
        tabInst.onClick = [this] { liveTab = false; update(); resized(); repaint(); };
        tabLive.onClick = [this] { liveTab = true; update(); resized(); repaint(); };
        tabLive.setVisible (proc.hasLiveInput());
        liveTab = proc.hasLiveInput() && proc.pv ("mode") == pl::ModeLive;
    }

    void showLive (bool b) { if (proc.hasLiveInput()) { liveTab = b; update(); resized(); repaint(); } }

    void update()
    {
        const int inst = (int) proc.pv ("inst");
        if (tiles.selected != inst) { tiles.selected = inst; tiles.repaint(); }
        const bool dim = proc.pv ("mode") == pl::ModeLive;
        if (tiles.dimmed != dim) { tiles.dimmed = dim; tiles.repaint(); }
        chA->setLabel (pl::charALabel (inst)); chB->setLabel (pl::charBLabel (inst));
        const bool struck = pl::isStruck (inst);
        dec->setLabel (struck ? "Length" : "Decay");
        sus->setEnabled (! struck); sus->setAlpha (struck ? 0.4f : 1.f);
        const bool va = pl::usesWave (inst) || inst == pl::Strings || inst == pl::Synth;
        det->setEnabled (va); det->setAlpha (va ? 1.f : 0.4f);
        const bool drums = inst == pl::Drums;
        for (auto* k : { cut, res, fenv, atk, rel, vib }) { k->setEnabled (! drums); k->setAlpha (drums ? 0.4f : 1.f); }
        wave.setVisible (! liveTab && pl::usesWave (inst));
        kit.setVisible (! liveTab && drums);
        waveLabel.setVisible (wave.isVisible() || kit.isVisible());
        waveLabel.setText (drums ? "KIT" : "WAVE", juce::dontSendNotification);
        if (inst != lastInst) { lastInst = inst; repaint(); }

        tabInst.setToggleState (! liveTab, juce::dontSendNotification);
        tabLive.setToggleState (liveTab, juce::dontSendNotification);
        tiles.setVisible (! liveTab);
        for (auto* k : knobs) k->setVisible (! liveTab);
        for (juce::Component* c : { (juce::Component*) gain.get(), (juce::Component*) &inCh, (juce::Component*) &tuner, (juce::Component*) &meter }) c->setVisible (liveTab);
    }

    void tick()
    {
        if (liveTab && isShowing())
        {
            meter.update();
            if (++tunerCount % 3 == 0)
            {
                const float f = proc.detectPitch();
                smoothed = f > 0.f && smoothed > 0.f && std::abs (f / smoothed - 1.f) < 0.03f ? smoothed * 0.6f + f * 0.4f : f;
                if (std::abs (tuner.freq - smoothed) > 0.01f) { tuner.freq = smoothed; tuner.repaint(); }
            }
        }
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        if (! proc.hasLiveInput())
        {
            g.setColour (Theme::violet); g.setFont (Theme::font (13.f, true));
            g.drawText ("INSTRUMENT", 18, 12, 200, 26, juce::Justification::centredLeft);
        }
        if (! liveTab)
        {
            drawSectionTitle (g, { 540, 10, 200, 16 }, "Sound");
            drawSectionTitle (g, { 540, 134, 200, 16 }, "Envelope");
            drawSectionTitle (g, { 540, 258, 200, 16 }, "Filter & Motion");
            g.setColour (Theme::border);
            g.drawVerticalLine (524, 14.f, (float) getHeight() - 14.f);
            const int inst = (int) proc.pv ("inst");
            g.setColour (Theme::text);
            g.setFont (Theme::font (15.f, true));
            g.drawText (pl::instNames()[inst], 16, 300, 300, 20, juce::Justification::centredLeft);
            g.setColour (Theme::dim);
            g.setFont (Theme::font (12.5f));
            g.drawFittedText (text::inst (inst), 16, 322, 492, 52, juce::Justification::topLeft, 3, 0.9f);
            if (! proc.hasLiveInput())
            {
                g.setColour (Theme::faint); g.setFont (Theme::font (11.f));
                g.drawText ("Live input (guitar, bass, mic): use PresetLab FX on an audio track, or the PresetLab app.", 16, getHeight() - 20, 500, 14, juce::Justification::centredLeft);
            }
        }
        else
        {
            g.setColour (Theme::border);
            g.drawVerticalLine (524, 14.f, (float) getHeight() - 14.f);
            drawSectionTitle (g, { 16, 52, 200, 16 }, "Input", Theme::cyan);
            drawSectionTitle (g, { 540, 52, 200, 16 }, "Tuner", Theme::cyan);
            g.setColour (Theme::dim); g.setFont (Theme::font (12.5f));
            g.drawFittedText ("Plug your guitar, bass or mic into your audio interface and pick its input. "
                              "In Ableton, put PresetLab FX on an audio track and set Monitor to In. "
                              "Pick Both in the top bar to play the instrument and your live input together.",
                              16, 262, 492, 70, juce::Justification::topLeft, 4, 0.9f);
            g.setColour (Theme::text.withAlpha (0.8f)); g.setFont (Theme::font (11.f, true));
            g.drawText ("CHANNEL", 16, 76, 120, 14, juce::Justification::centredLeft);
            g.drawText ("LEVEL", 180, 76, 120, 14, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        tabInst.setBounds (16, 12, 120, 26);
        tabLive.setBounds (142, 12, 120, 26);
        tiles.setBounds (16, 48, 492, 214);
        waveLabel.setBounds (16, 270, 50, 22);
        wave.setBounds (60, 268, 150, 26); kit.setBounds (60, 268, 150, 26);
        const int x0 = 534, cw = 78, ch = 104;
        auto place = [&] (std::initializer_list<Knob*> row, int y) { int x = x0; for (auto* k : row) { k->setBounds (x, y, cw, ch); x += cw; } };
        place ({ tone, chA, chB, level, oct, fine }, 26);
        place ({ atk, dec, sus, rel, vel, width }, 150);
        place ({ cut, res, fenv, det, vib }, 274);
        inCh.setBounds (16, 94, 150, 28);
        gain->setBounds (180, 92, 90, 110);
        meter.setBounds (16, 140, 150, 10);
        tuner.setBounds (540, 70, getWidth() - 556, 300);
    }

private:
    PresetLabProcessor& proc;
    InstTiles tiles;
    juce::OwnedArray<Knob> knobs;
    Knob* tone, * chA, * chB, * level, * oct, * fine, * atk, * dec, * sus, * rel, * vel, * width, * cut, * res, * fenv, * det, * vib;
    ChoiceBox wave, kit, inCh;
    juce::Label waveLabel;
    std::unique_ptr<Knob> gain;
    TunerView tuner;
    Meter meter;
    juce::TextButton tabInst, tabLive;
    bool liveTab = false;
    int lastInst = -1, tunerCount = 0;
    float smoothed = 0.f;
};

// ================================================================================================
// Stomp (pedal) or rack slot
class FxPage : public juce::Component
{
public:
    FxPage (PresetLabProcessor& p, int s) : proc (p), slot (s)
    {
        auto& st = p.apvts;
        for (int k = 0; k < 4; ++k) { knobs[k] = std::make_unique<Knob> (st, pl::slotId (s, pl::knobKey (k)), "", Theme::accent); addAndMakeVisible (*knobs[k]); }
        mix = std::make_unique<Knob> (st, pl::slotId (s, "mix"), "Mix", Theme::accent);
        addAndMakeVisible (*mix);
        power.foot = s < 3;
        addAndMakeVisible (power);
        powerAtt = std::make_unique<APVTS::ButtonAttachment> (st, pl::slotId (s, "on"), power);
        for (int t = 0; t < pl::kNumFx; ++t) types.items.push_back ({ pl::fxInfo (t).name, juce::Colour (pl::fxInfo (t).colour) });
        types.cols = s < 3 ? 4 : 6; types.rowH = 30;
        types.onPick = [this] (int t)
        {
            proc.undo.beginNewTransaction();
            const int cur = (int) proc.pv (pl::slotId (slot, "type"));
            if (t != cur)
            {
                proc.setParam (pl::slotId (slot, "type"), (float) t);
                for (int k = 0; k < 4; ++k) proc.setParam (pl::slotId (slot, pl::knobKey (k)), pl::fxInfo (t).def[k]);
                proc.setParam (pl::slotId (slot, "mix"), pl::fxInfo (t).mix);
            }
            proc.setParam (pl::slotId (slot, "on"), 1.f);
        };
        addAndMakeVisible (types);
    }

    void update()
    {
        const int t = (int) proc.pv (pl::slotId (slot, "type"));
        const bool on = proc.pv (pl::slotId (slot, "on")) > 0.5f;
        const auto& info = pl::fxInfo (t);
        const juce::Colour c (info.colour);
        for (int k = 0; k < 4; ++k)
        {
            knobs[k]->setVisible (info.knobs[k] != nullptr);
            if (info.knobs[k] != nullptr) knobs[k]->setLabel (info.knobs[k]);
            knobs[k]->setKnobColour (c);
        }
        mix->setKnobColour (c);
        power.colour = c;
        if (types.selected != t) { types.selected = t; types.repaint(); }
        if (t != lastType || on != lastOn) { lastType = t; lastOn = on; resized(); repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        const int t = (int) proc.pv (pl::slotId (slot, "type"));
        const bool on = proc.pv (pl::slotId (slot, "on")) > 0.5f;
        const auto& info = pl::fxInfo (t);
        const juce::Colour c (info.colour);
        const juce::String title = slot < 3 ? "STOMP " + juce::String (slot + 1) : "RACK " + juce::String (slot - 2);

        if (slot < 3)
        {
            // pedal
            auto pr = pedalRect();
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRoundedRectangle (pr.translated (0.f, 6.f), 18.f);
            g.setGradientFill (juce::ColourGradient (c.brighter (0.15f), pr.getX(), pr.getY(), c.darker (0.45f), pr.getRight(), pr.getBottom(), false));
            g.fillRoundedRectangle (pr, 18.f);
            g.setColour (juce::Colours::white.withAlpha (0.25f));
            g.drawRoundedRectangle (pr.reduced (1.f), 17.f, 1.2f);
            // screws
            for (auto pt : { pr.getTopLeft() + juce::Point<float> (14, 14), pr.getTopRight() + juce::Point<float> (-14, 14),
                             pr.getBottomLeft() + juce::Point<float> (14, -14), pr.getBottomRight() + juce::Point<float> (-14, -14) })
            {
                g.setColour (juce::Colours::black.withAlpha (0.35f)); g.fillEllipse (juce::Rectangle<float> (8.f, 8.f).withCentre (pt));
            }
            // knob well
            auto well = juce::Rectangle<float> (pr.getX() + 14.f, pr.getY() + 26.f, pr.getWidth() - 28.f, 200.f);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillRoundedRectangle (well, 12.f);
            // LED
            auto led = juce::Rectangle<float> (10.f, 10.f).withCentre ({ pr.getCentreX(), pr.getY() + 14.f });
            if (on) for (int i = 3; i >= 1; --i) { g.setColour (Theme::red.withAlpha (0.15f * (float) i)); g.fillEllipse (led.expanded ((float) (4 - i) * 2.f)); }
            g.setColour (on ? Theme::red : juce::Colour (0xff4a1f24)); g.fillEllipse (led);
            // name plate
            const bool darkText = c.getPerceivedBrightness() > 0.7f;
            g.setColour (darkText ? juce::Colours::black.withAlpha (0.85f) : juce::Colours::white);
            g.setFont (Theme::font (20.f, true));
            g.drawFittedText (juce::String (info.name).toUpperCase(), juce::Rectangle<int> ((int) pr.getX() + 10, (int) well.getBottom() + 8, (int) pr.getWidth() - 20, 26), juce::Justification::centred, 1);
            g.setFont (Theme::font (10.f, true));
            g.setColour ((darkText ? juce::Colours::black : juce::Colours::white).withAlpha (0.6f));
            g.drawText ("PRESETLAB", juce::Rectangle<float> (pr.getX(), well.getBottom() + 32.f, pr.getWidth(), 14.f), juce::Justification::centred);

            // right side
            g.setColour (c); g.setFont (Theme::font (13.f, true));
            g.drawText (title, 372, 12, 200, 20, juce::Justification::centredLeft);
            g.setColour (Theme::dim); g.setFont (Theme::font (12.f));
            g.drawText ("Pick an effect. Stomps run before the amp.", 450, 12, 400, 20, juce::Justification::centredLeft);
            g.setColour (Theme::text); g.setFont (Theme::font (15.f, true));
            g.drawText (info.name, 372, 280, 400, 20, juce::Justification::centredLeft);
            g.setColour (Theme::dim); g.setFont (Theme::font (12.5f));
            g.drawFittedText (text::fx (t) + (on ? "" : "  (Off: click the footswitch to turn it on.)"), 372, 302, 520, 40, juce::Justification::topLeft, 2, 0.9f);
        }
        else
        {
            auto rr = rackRect();
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRoundedRectangle (rr.translated (0.f, 5.f), 8.f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c313b), 0.f, rr.getY(), juce::Colour (0xff15181e), 0.f, rr.getBottom(), false));
            g.fillRoundedRectangle (rr, 8.f);
            for (auto ear : { rr.withWidth (34.f), rr.withTrimmedLeft (rr.getWidth() - 34.f) })
            {
                g.setColour (juce::Colour (0xff3a404c)); g.fillRoundedRectangle (ear, 6.f);
                for (float fy : { 0.22f, 0.78f })
                {
                    auto h = juce::Rectangle<float> (14.f, 8.f).withCentre ({ ear.getCentreX(), ear.getY() + ear.getHeight() * fy });
                    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.fillRoundedRectangle (h, 4.f);
                }
            }
            g.setColour (c); g.fillRect (rr.getX() + 34.f, rr.getY(), 4.f, rr.getHeight());
            g.setColour (Theme::text); g.setFont (Theme::font (22.f, true));
            g.drawText (juce::String (info.name).toUpperCase(), juce::Rectangle<float> (rr.getX() + 52.f, rr.getY() + 34.f, 230.f, 30.f), juce::Justification::centredLeft);
            g.setColour (c); g.setFont (Theme::font (11.f, true));
            g.drawText (title + juce::String (juce::CharPointer_UTF8 ("  \xc2\xb7  PRESETLAB")), juce::Rectangle<float> (rr.getX() + 52.f, rr.getY() + 66.f, 230.f, 16.f), juce::Justification::centredLeft);
            auto led = juce::Rectangle<float> (8.f, 8.f).withCentre ({ rr.getX() + 58.f, rr.getY() + 24.f });
            g.setColour (on ? c : Theme::faint); g.fillEllipse (led);
            g.setColour (Theme::dim); g.setFont (Theme::font (12.5f));
            g.drawFittedText (text::fx (t), (int) rr.getX() + 52, (int) rr.getY() + 92, 240, 44, juce::Justification::topLeft, 3, 0.9f);
            g.setColour (Theme::dim); g.setFont (Theme::font (12.f));
            g.drawText ("Pick an effect. Rack effects run after the cab.", 16, 206, 500, 18, juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        const int t = (int) proc.pv (pl::slotId (slot, "type"));
        const auto& info = pl::fxInfo (t);
        int vis[4], nv = 0;
        for (int k = 0; k < 4; ++k) if (info.knobs[k] != nullptr) vis[nv++] = k;
        if (slot < 3)
        {
            auto pr = pedalRect();
            const int kw = 86, kh = 96;
            const int cx = (int) pr.getCentreX(), top = (int) pr.getY() + 32;
            for (int i = 0; i < nv; ++i)
            {
                const int rowCount = nv <= 2 ? nv : (i < 2 ? 2 : nv - 2);
                const int row = nv <= 2 ? 0 : i / 2, col = nv <= 2 ? i : i % 2;
                const int y = nv <= 2 ? top + 50 : top + row * kh;
                const int x = cx - rowCount * kw / 2 + col * kw;
                knobs[vis[i]]->setBounds (x, y, kw, kh);
            }
            power.setBounds (juce::Rectangle<int> (62, 62).withCentre ({ cx, (int) pr.getBottom() - 46 }));
            types.setBounds (372, 40, getWidth() - 388, types.heightFor (getWidth() - 388));
            mix->setBounds (920, 270, 86, 100);
        }
        else
        {
            auto rr = rackRect();
            int x = (int) rr.getX() + 300;
            for (int i = 0; i < nv; ++i) { knobs[vis[i]]->setBounds (x, (int) rr.getY() + 22, 96, 108); x += 104; }
            mix->setBounds ((int) rr.getRight() - 200, (int) rr.getY() + 22, 96, 108);
            power.setBounds ((int) rr.getRight() - 84, (int) rr.getCentreY() - 20, 40, 40);
            types.setBounds (16, 230, getWidth() - 32, types.heightFor (getWidth() - 32));
        }
    }

private:
    juce::Rectangle<float> pedalRect() const { return { 70.f, 22.f, 230.f, 346.f }; }
    juce::Rectangle<float> rackRect() const { return { 16.f, 30.f, (float) getWidth() - 32.f, 150.f }; }

    PresetLabProcessor& proc;
    int slot;
    std::unique_ptr<Knob> knobs[4], mix;
    PowerButton power;
    std::unique_ptr<APVTS::ButtonAttachment> powerAtt;
    PillGrid types;
    int lastType = -1; bool lastOn = false;
};

// ================================================================================================
class AmpPage : public juce::Component
{
public:
    explicit AmpPage (PresetLabProcessor& p) : proc (p)
    {
        for (int i = 0; i < 6; ++i) { knobs[i] = std::make_unique<Knob> (p.apvts, pl::ampId (i), pl::ampKnob (i), Theme::amber); addAndMakeVisible (*knobs[i]); }
        addAndMakeVisible (power);
        powerAtt = std::make_unique<APVTS::ButtonAttachment> (p.apvts, "amp_on", power);
        for (int m = 0; m < pl::ampNames().size(); ++m) models.items.push_back ({ pl::ampNames()[m], text::ampColour (m) });
        models.cols = 5;
        models.onPick = [this] (int m) { proc.undo.beginNewTransaction(); proc.setParam ("amp_model", (float) m); proc.setParam ("amp_on", 1.f); };
        addAndMakeVisible (models);
    }
    void update()
    {
        const int m = (int) proc.pv ("amp_model");
        const bool on = proc.pv ("amp_on") > 0.5f;
        const auto c = text::ampColour (m);
        const bool light = c.getPerceivedBrightness() > 0.55f;
        power.colour = Theme::amber;
        for (auto& k : knobs)
        {
            k->setKnobColour (m == 4 ? Theme::red : Theme::amber);
            k->label.setColour (juce::Label::textColourId, light ? juce::Colours::black.withAlpha (0.8f) : juce::Colours::white.withAlpha (0.85f));
            k->slider.setColour (juce::Slider::textBoxTextColourId, light ? juce::Colours::black.withAlpha (0.65f) : juce::Colours::white.withAlpha (0.6f));
        }
        if (models.selected != m || on != lastOn) { models.selected = m; lastOn = on; models.repaint(); repaint(); }
    }
    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        const int m = (int) proc.pv ("amp_model");
        const bool on = proc.pv ("amp_on") > 0.5f;
        const auto c = text::ampColour (m);
        auto head = headRect();
        // handle
        g.setColour (juce::Colour (0xff15171b));
        g.fillRoundedRectangle (juce::Rectangle<float> (180.f, 16.f).withCentre ({ head.getCentreX(), head.getY() - 6.f }), 7.f);
        // tolex
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (head.translated (0.f, 6.f), 16.f);
        g.setColour (m == 1 || m == 7 ? juce::Colour (0xff3b2a20) : juce::Colour (0xff17191e));
        g.fillRoundedRectangle (head, 16.f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (head.reduced (6.f), 12.f, 1.f);
        // grille
        auto grille = head.reduced (14.f).removeFromTop (78.f);
        g.setColour (juce::Colour (0xff23262d)); g.fillRoundedRectangle (grille, 6.f);
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        for (float x = grille.getX(); x < grille.getRight(); x += 5.f) g.drawVerticalLine ((int) x, grille.getY(), grille.getBottom());
        drawLogoIcon (g, juce::Rectangle<float> (40.f, 40.f).withCentre ({ grille.getX() + 44.f, grille.getCentreY() }));
        drawWordmark (g, juce::Rectangle<float> (grille.getX() + 74.f, grille.getY(), 200.f, grille.getHeight()), 24.f);
        g.setColour (juce::Colours::white.withAlpha (0.85f)); g.setFont (Theme::font (24.f, true));
        g.drawText (pl::ampNames()[m].toUpperCase(), grille.withTrimmedRight (24.f), juce::Justification::centredRight);
        // control panel
        auto panelR = head.reduced (14.f).withTrimmedTop (86.f);
        g.setGradientFill (juce::ColourGradient (c.brighter (0.1f), 0.f, panelR.getY(), c.darker (0.25f), 0.f, panelR.getBottom(), false));
        g.fillRoundedRectangle (panelR, 6.f);
        g.setColour (juce::Colours::black.withAlpha (0.25f)); g.drawRoundedRectangle (panelR, 6.f, 1.f);
        // jewel light
        auto jewel = juce::Rectangle<float> (18.f, 18.f).withCentre ({ panelR.getRight() - 110.f, panelR.getCentreY() });
        if (on) for (int i = 3; i >= 1; --i) { g.setColour (Theme::red.withAlpha (0.14f * (float) i)); g.fillEllipse (jewel.expanded ((float) (4 - i) * 3.f)); }
        g.setColour (on ? Theme::red : juce::Colour (0xff4a1f24)); g.fillEllipse (jewel);
        g.setColour (Theme::dim); g.setFont (Theme::font (12.5f));
        g.drawText (text::amp (m) + (on ? "" : "  The amp is off: press the power button to use it."), 30, 340, getWidth() - 60, 20, juce::Justification::centredLeft);
    }
    void resized() override
    {
        auto head = headRect();
        auto panelR = head.reduced (14.f).withTrimmedTop (86.f);
        int x = (int) panelR.getX() + 20;
        for (auto& k : knobs) { k->setBounds (x, (int) panelR.getY() + 4, 92, (int) panelR.getHeight() - 8); x += 100; }
        power.setBounds ((int) panelR.getRight() - 70, (int) panelR.getCentreY() - 21, 42, 42);
        models.setBounds (30, 262, getWidth() - 60, models.heightFor (getWidth() - 60));
    }
private:
    juce::Rectangle<float> headRect() const { return { 60.f, 30.f, (float) getWidth() - 120.f, 210.f }; }
    PresetLabProcessor& proc;
    std::unique_ptr<Knob> knobs[6];
    PowerButton power;
    std::unique_ptr<APVTS::ButtonAttachment> powerAtt;
    PillGrid models;
    bool lastOn = false;
};

// ================================================================================================
class CabPage : public juce::Component
{
public:
    explicit CabPage (PresetLabProcessor& p) : proc (p)
    {
        mic = std::make_unique<Knob> (p.apvts, "cab_mic", "Mic Position", Theme::amber);
        room = std::make_unique<Knob> (p.apvts, "cab_room", "Room", Theme::amber);
        level = std::make_unique<Knob> (p.apvts, "cab_level", "Level", Theme::amber, true);
        for (auto* k : { mic.get(), room.get(), level.get() }) addAndMakeVisible (k);
        addAndMakeVisible (power);
        power.colour = Theme::amber;
        powerAtt = std::make_unique<APVTS::ButtonAttachment> (p.apvts, "cab_on", power);
        for (int m = 0; m < pl::cabNames().size(); ++m) models.items.push_back ({ pl::cabNames()[m], juce::Colour (0xffc08a5a) });
        models.cols = 2;
        models.onPick = [this] (int m) { proc.undo.beginNewTransaction(); proc.setParam ("cab_model", (float) m); proc.setParam ("cab_on", 1.f); };
        addAndMakeVisible (models);
    }
    void update()
    {
        const int m = (int) proc.pv ("cab_model");
        const float mp = proc.pv ("cab_mic");
        const bool on = proc.pv ("cab_on") > 0.5f;
        if (models.selected != m || std::abs (mp - lastMic) > 0.001f || on != lastOn) { models.selected = m; lastMic = mp; lastOn = on; models.repaint(); repaint(); }
    }
    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        const int m = (int) proc.pv ("cab_model");
        const bool on = proc.pv ("cab_on") > 0.5f;
        auto cab = juce::Rectangle<float> (40.f, 30.f, 320.f, 330.f);
        if (m == 9) cab = cab.withSizeKeepingCentre (300.f, 200.f);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (cab.translated (0.f, 6.f), 14.f);
        g.setColour (m == 4 ? juce::Colour (0xffc9ae7c) : m == 8 ? juce::Colour (0xff3b2a20) : m == 9 ? juce::Colour (0xff6b4a2e) : juce::Colour (0xff17191e));
        g.fillRoundedRectangle (cab, 14.f);
        auto grille = cab.reduced (16.f);
        g.setColour (m == 4 ? juce::Colour (0xff8a7356) : juce::Colour (0xff24272e)); g.fillRoundedRectangle (grille, 8.f);
        // speakers
        struct L { int n, cols; float size; };
        static const L lay[] { { 1, 1, 0.8f }, { 2, 2, 0.48f }, { 4, 2, 0.46f }, { 4, 2, 0.46f }, { 1, 1, 0.78f }, { 1, 1, 0.9f }, { 4, 2, 0.44f }, { 8, 2, 0.24f }, { 2, 1, 0.42f }, { 1, 1, 0.6f } };
        const auto& L0 = lay[juce::jlimit (0, 9, m)];
        const int rows = (L0.n + L0.cols - 1) / L0.cols;
        juce::Point<float> first;
        for (int i = 0; i < L0.n; ++i)
        {
            const float cw = grille.getWidth() / (float) L0.cols, chh = grille.getHeight() / (float) rows;
            const auto ctr = juce::Point<float> (grille.getX() + cw * ((float) (i % L0.cols) + 0.5f), grille.getY() + chh * ((float) (i / L0.cols) + 0.5f));
            const float d = std::min (grille.getWidth(), grille.getHeight()) * L0.size;
            if (i == 0) first = ctr;
            auto sp = juce::Rectangle<float> (d, d).withCentre (ctr);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3e47), sp.getX(), sp.getY(), juce::Colour (0xff0d0e11), sp.getRight(), sp.getBottom(), false));
            g.fillEllipse (sp);
            g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (sp, 2.f);
            auto cone = sp.reduced (d * 0.12f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2b2e35), cone.getCentreX(), cone.getCentreY(), juce::Colour (0xff121317), cone.getRight(), cone.getBottom(), true));
            g.fillEllipse (cone);
            auto dust = juce::Rectangle<float> (d * 0.28f, d * 0.28f).withCentre (ctr);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff50555f), dust.getX(), dust.getY(), juce::Colour (0xff1c1e23), dust.getRight(), dust.getBottom(), false));
            g.fillEllipse (dust);
        }
        // mic over the first speaker: centre (bright) to edge (dark)
        {
            const float d = std::min (grille.getWidth(), grille.getHeight()) * L0.size;
            const float off = proc.pv ("cab_mic") * d * 0.45f;
            const auto mp = first + juce::Point<float> (off, -off * 0.2f);
            g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillEllipse (juce::Rectangle<float> (26.f, 26.f).withCentre (mp + juce::Point<float> (2.f, 3.f)));
            g.setColour (juce::Colour (0xff9aa3b2)); g.fillEllipse (juce::Rectangle<float> (24.f, 24.f).withCentre (mp));
            g.setColour (juce::Colour (0xff3b4150)); g.fillEllipse (juce::Rectangle<float> (14.f, 14.f).withCentre (mp));
            g.setColour (Theme::amber.withAlpha (0.8f)); g.drawEllipse (juce::Rectangle<float> (24.f, 24.f).withCentre (mp), 1.5f);
        }
        if (! on) { g.setColour (Theme::panel.withAlpha (0.55f)); g.fillRoundedRectangle (cab, 14.f); }
        g.setColour (Theme::amber); g.setFont (Theme::font (13.f, true));
        g.drawText ("CABINET", 400, 12, 200, 20, juce::Justification::centredLeft);
        g.setColour (Theme::dim); g.setFont (Theme::font (12.f));
        g.drawText ("The speaker and mic the amp plays through.", 470, 12, 400, 20, juce::Justification::centredLeft);
        g.setColour (Theme::text); g.setFont (Theme::font (15.f, true));
        g.drawText (pl::cabNames()[m], 400, 240, 400, 20, juce::Justification::centredLeft);
        g.setColour (Theme::dim); g.setFont (Theme::font (12.5f));
        g.drawFittedText (text::cab (m) + " Mic Position moves the mic from the centre (bright) to the edge (dark).", 400, 262, 300, 70, juce::Justification::topLeft, 4, 0.9f);
    }
    void resized() override
    {
        models.setBounds (400, 40, 600, models.heightFor (600));
        mic->setBounds (720, 250, 90, 108); room->setBounds (812, 250, 90, 108); level->setBounds (904, 250, 90, 108);
        power.setBounds (960, 8, 36, 36);
    }
private:
    PresetLabProcessor& proc;
    std::unique_ptr<Knob> mic, room, level;
    PowerButton power;
    std::unique_ptr<APVTS::ButtonAttachment> powerAtt;
    PillGrid models;
    float lastMic = -1.f; bool lastOn = false;
};
