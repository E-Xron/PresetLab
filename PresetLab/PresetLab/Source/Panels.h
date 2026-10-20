#pragma once
#include "Pages.h"

// ================================================================================================
// The signal chain across the top: Source > 3 stomps > Amp > Cab > 3 rack effects. Click a block to edit it.
class ChainStrip : public juce::Component
{
public:
    explicit ChainStrip (PresetLabProcessor& p) : proc (p) {}
    int selected = 0;
    std::function<void (int)> onSelect;

    struct Info { juce::String title, sub; juce::Colour colour; bool hasPower, on; };
    Info info (int b) const
    {
        if (b == 0)
        {
            const int mode = (int) proc.pv ("mode");
            const juce::String inst = pl::instShort ((int) proc.pv ("inst"));
            return { mode == pl::ModeLive ? "Live Input" : mode == pl::ModeBoth ? inst + " + Input" : inst, "SOURCE", Theme::violet, false, true };
        }
        if (b == 4) return { pl::ampNames()[(int) proc.pv ("amp_model")], "AMP", Theme::amber, true, proc.pv ("amp_on") > 0.5f };
        if (b == 5) return { pl::cabNames()[(int) proc.pv ("cab_model")], "CAB", juce::Colour (0xffc08a5a), true, proc.pv ("cab_on") > 0.5f };
        const int s = slotOf (b);
        const auto& fi = pl::fxInfo ((int) proc.pv (pl::slotId (s, "type")));
        return { fi.name, s < 3 ? "STOMP " + juce::String (s + 1) : "RACK " + juce::String (s - 2), juce::Colour (fi.colour), true, proc.pv (pl::slotId (s, "on")) > 0.5f };
    }
    static int slotOf (int b) { return b <= 3 ? b - 1 : b - 3; }

    juce::Rectangle<float> block (int b) const
    {
        const float gap = 16.f, w = ((float) getWidth() - 24.f - gap * 8.f) / 9.f;
        return { 12.f + (float) b * (w + gap), 24.f, w, (float) getHeight() - 32.f };
    }
    juce::Rectangle<float> led (int b) const { auto r = block (b); return { r.getRight() - 22.f, r.getY() + 6.f, 16.f, 16.f }; }

    void refresh()
    {
        juce::String key;
        for (int b = 0; b < 9; ++b) { const auto i = info (b); key << i.title << (int) i.on; }
        key << selected;
        if (key != lastKey) { lastKey = key; repaint(); }
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        auto label = [&] (int b0, int b1, const char* t)
        {
            auto a = block (b0), z = block (b1);
            drawSectionTitle (g, juce::Rectangle<float> (a.getX(), 4.f, z.getRight() - a.getX(), 18.f).toNearestInt(), t);
        };
        label (0, 0, "Source"); label (1, 3, "Stomps"); label (4, 4, "Amp"); label (5, 5, "Cab"); label (6, 8, "Rack");

        for (int b = 0; b < 9; ++b)
        {
            auto r = block (b);
            const auto in = info (b);
            const bool sel = b == selected, hov = b == hover;
            // arrow to the next block
            if (b < 8)
            {
                const float x0 = r.getRight() + 3.f, x1 = r.getRight() + 13.f, y = r.getCentreY();
                g.setColour (Theme::faint);
                g.drawLine (x0, y, x1 - 3.f, y, 1.5f);
                juce::Path a; a.addTriangle (x1, y, x1 - 5.f, y - 4.f, x1 - 5.f, y + 4.f);
                g.fillPath (a);
            }
            const float alpha = in.on ? 1.f : 0.45f;
            g.setColour (sel ? in.colour.withAlpha (0.16f) : Theme::panel2.brighter (hov ? 0.06f : 0.f));
            g.fillRoundedRectangle (r, 10.f);
            if (sel)
            {
                g.setColour (in.colour.withAlpha (0.25f)); g.drawRoundedRectangle (r.expanded (2.f), 12.f, 3.f);
                g.setColour (in.colour); g.drawRoundedRectangle (r.reduced (0.5f), 10.f, 1.5f);
            }
            else { g.setColour (Theme::border); g.drawRoundedRectangle (r.reduced (0.5f), 10.f, 1.f); }
            g.setColour (in.colour.withAlpha (alpha));
            g.fillRoundedRectangle (r.getX() + 10.f, r.getY() + 0.5f, r.getWidth() - 20.f, 3.f, 1.5f);

            auto icon = juce::Rectangle<float> (30.f, 30.f).withCentre ({ r.getCentreX(), r.getY() + 24.f });
            drawBlockIcon (g, b, icon, in.colour.withAlpha (alpha));
            g.setColour (Theme::text.withAlpha (alpha));
            g.setFont (Theme::font (11.5f, true));
            g.drawFittedText (in.title, r.withTrimmedTop (42.f).withHeight (16.f).reduced (4.f, 0.f).toNearestInt(), juce::Justification::centred, 1, 0.75f);
            g.setColour (Theme::dim.withAlpha (alpha));
            g.setFont (Theme::font (9.5f, true));
            g.drawText (in.sub, r.withTrimmedTop (58.f).withHeight (13.f), juce::Justification::centred);
            if (in.hasPower)
            {
                auto l = led (b).reduced (4.f);
                if (in.on) { g.setColour (in.colour.withAlpha (0.3f)); g.fillEllipse (l.expanded (3.f)); }
                g.setColour (in.on ? in.colour : Theme::panel3.brighter (0.2f));
                g.fillEllipse (l);
            }
        }
    }

    void mouseMove (const juce::MouseEvent& e) override { int h = hitBlock (e.position); if (h != hover) { hover = h; repaint(); } }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int b = hitBlock (e.position);
        if (b < 0) return;
        if (e.mods.isPopupMenu()) { showMenu (b); return; }
        if (info (b).hasPower && led (b).expanded (4.f).contains (e.position)) { togglePower (b); return; }
        selected = b;
        if (onSelect) onSelect (b);
        repaint();
    }
    void mouseDoubleClick (const juce::MouseEvent& e) override { const int b = hitBlock (e.position); if (b > 0 && ! led (b).expanded (4.f).contains (e.position)) togglePower (b); }

private:
    int hitBlock (juce::Point<float> p) const { for (int b = 0; b < 9; ++b) if (block (b).contains (p)) return b; return -1; }
    juce::String powerId (int b) const { return b == 4 ? "amp_on" : b == 5 ? "cab_on" : pl::slotId (slotOf (b), "on"); }
    void togglePower (int b)
    {
        proc.undo.beginNewTransaction();
        proc.setParam (powerId (b), proc.pv (powerId (b)) > 0.5f ? 0.f : 1.f);
        repaint();
    }
    void showMenu (int b)
    {
        juce::PopupMenu m;
        juce::StringArray names; juce::String id;
        if (b == 0) { names = pl::instNames(); id = "inst"; }
        else if (b == 4) { names = pl::ampNames(); id = "amp_model"; }
        else if (b == 5) { names = pl::cabNames(); id = "cab_model"; }
        else { names = pl::fxNames(); id = pl::slotId (slotOf (b), "type"); }
        const int cur = (int) proc.pv (id);
        if (b > 0) { m.addItem (1000, info (b).on ? "Turn off" : "Turn on"); m.addSeparator(); }
        for (int i = 0; i < names.size(); ++i) m.addItem (i + 1, names[i], true, i == cur);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetScreenArea (localAreaToGlobal (block (b).toNearestInt())), [this, b, id] (int r)
        {
            if (r <= 0) return;
            if (r == 1000) { togglePower (b); return; }
            proc.undo.beginNewTransaction();
            if (b > 0 && b != 4 && b != 5)
            {
                const int t = r - 1, s = slotOf (b);
                for (int k = 0; k < 4; ++k) proc.setParam (pl::slotId (s, pl::knobKey (k)), pl::fxInfo (t).def[k]);
                proc.setParam (pl::slotId (s, "mix"), pl::fxInfo (t).mix);
            }
            proc.setParam (id, (float) (r - 1));
            if (b > 0) proc.setParam (powerId (b), 1.f);
            selected = b; if (onSelect) onSelect (b);
        });
    }

    void drawBlockIcon (juce::Graphics& g, int b, juce::Rectangle<float> r, juce::Colour c)
    {
        g.setColour (c);
        if (b == 0)
        {
            if ((int) proc.pv ("mode") == pl::ModeLive)
            {   // input jack
                g.drawEllipse (r.reduced (6.f), 2.f);
                g.fillEllipse (r.reduced (11.f));
                g.drawLine (r.getX(), r.getCentreY(), r.getX() + 6.f, r.getCentreY(), 2.f);
            }
            else drawInstIcon (g, (int) proc.pv ("inst"), r, c);
        }
        else if (b == 4)
        {
            auto h = r.reduced (1.f, 6.f);
            g.drawRoundedRectangle (h, 3.f, 1.6f);
            for (int i = 0; i < 4; ++i) g.fillEllipse (juce::Rectangle<float> (4.f, 4.f).withCentre ({ h.getX() + 6.f + (float) i * 6.f, h.getBottom() - 5.f }));
            g.fillRect (h.getX() + 3.f, h.getY() + 3.f, h.getWidth() - 6.f, 4.f);
        }
        else if (b == 5)
        {
            g.drawRoundedRectangle (r.reduced (2.f), 3.f, 1.6f);
            for (int i = 0; i < 4; ++i) g.drawEllipse (juce::Rectangle<float> (9.f, 9.f).withCentre ({ r.getCentreX() + (i % 2 ? 5.5f : -5.5f), r.getCentreY() + (i / 2 ? 5.5f : -5.5f) }), 1.4f);
        }
        else if (b <= 3)
        {
            auto p = r.reduced (6.f, 1.f);
            g.drawRoundedRectangle (p, 4.f, 1.6f);
            g.fillEllipse (juce::Rectangle<float> (5.f, 5.f).withCentre ({ p.getX() + 6.f, p.getY() + 7.f }));
            g.fillEllipse (juce::Rectangle<float> (5.f, 5.f).withCentre ({ p.getRight() - 6.f, p.getY() + 7.f }));
            g.drawEllipse (juce::Rectangle<float> (8.f, 8.f).withCentre ({ p.getCentreX(), p.getBottom() - 7.f }), 1.4f);
        }
        else
        {
            auto p = r.reduced (0.f, 8.f);
            g.drawRoundedRectangle (p, 2.f, 1.6f);
            for (int i = 0; i < 3; ++i) g.fillEllipse (juce::Rectangle<float> (4.f, 4.f).withCentre ({ p.getX() + 10.f + (float) i * 6.f, p.getCentreY() }));
            g.fillRect (p.getX() + 2.f, p.getY() + 2.f, 3.f, p.getHeight() - 4.f);
        }
    }

    PresetLabProcessor& proc;
    int hover = -1;
    juce::String lastKey;
};

// ================================================================================================
class ChordPad : public juce::Component
{
public:
    juce::String name, roman;
    juce::Colour colour = Theme::violet;
    bool lit = false, empty = false;
    std::function<void (bool)> onPress;
    std::function<void()> onMenu;

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = lit || down;
        if (empty)
        {
            g.setColour (Theme::border);
            float dashes[] { 4.f, 4.f };
            juce::Path p; p.addRoundedRectangle (r, 9.f);
            juce::Path d; juce::PathStrokeType (1.f).createDashedStroke (d, p, dashes, 2);
            g.fillPath (d);
            g.setColour (Theme::faint); g.setFont (Theme::font (16.f));
            g.drawText ("+", r, juce::Justification::centred);
            return;
        }
        if (on) g.setGradientFill (juce::ColourGradient (colour.brighter (0.2f), 0.f, r.getY(), colour.darker (0.3f), 0.f, r.getBottom(), false));
        else g.setColour (hover ? Theme::panel3.brighter (0.05f) : Theme::panel3);
        g.fillRoundedRectangle (r, 9.f);
        g.setColour (on ? colour.brighter (0.5f) : colour.withAlpha (hover ? 0.8f : 0.45f));
        g.drawRoundedRectangle (r, 9.f, on ? 1.5f : 1.f);
        g.setColour (on ? juce::Colours::white : Theme::text);
        g.setFont (Theme::font (r.getHeight() > 40.f ? 16.f : 14.f, true));
        g.drawFittedText (name, r.withTrimmedBottom (r.getHeight() * 0.36f).toNearestInt().reduced (4, 0), juce::Justification::centredBottom, 1, 0.7f);
        g.setColour (on ? juce::Colours::white.withAlpha (0.8f) : colour.withAlpha (0.9f));
        g.setFont (Theme::font (10.5f, true));
        g.drawText (roman, r.withTrimmedTop (r.getHeight() * 0.62f), juce::Justification::centredTop);
    }
    void mouseEnter (const juce::MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const juce::MouseEvent&) override { hover = false; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || empty) { if (onMenu) onMenu(); return; }
        down = true; repaint();
        if (onPress) onPress (true);
    }
    void mouseUp (const juce::MouseEvent&) override
    {
        if (! down) return;
        down = false; repaint();
        if (onPress) onPress (false);
    }
private:
    bool down = false, hover = false;
};

class DragMidiButton : public juce::TextButton
{
public:
    std::function<juce::File()> makeFile;
    std::function<void()> onPlainClick;
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging || e.getDistanceFromDragStart() < 6 || ! makeFile) return;
        dragging = true;
        const auto f = makeFile();
        if (f.existsAsFile())
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this, [this] { dragging = false; });
        else dragging = false;
    }
    void mouseUp (const juce::MouseEvent& e) override
    {
        const bool wasDrag = e.getDistanceFromDragStart() >= 6;
        juce::TextButton::mouseUp (e);
        if (! wasDrag && onPlainClick && isMouseOver()) onPlainClick();
        dragging = false;
    }
private:
    bool dragging = false;
};

// ================================================================================================
class ChordPanel : public juce::Component
{
public:
    explicit ChordPanel (PresetLabProcessor& p) : proc (p),
        mode (p.apvts, "ch_mode"), key (p.apvts, "ch_key"), scale (p.apvts, "ch_scale"), ext (p.apvts, "ch_ext"), type (p.apvts, "ch_type"),
        voicing (p.apvts, "ch_voicing"), inv (p.apvts, "ch_inv"), dir (p.apvts, "ch_dir")
    {
        auto& s = p.apvts;
        power.setButtonText ("CHORDS");
        power.setColour (juce::ToggleButton::tickColourId, Theme::pink);
        powerAtt = std::make_unique<APVTS::ButtonAttachment> (s, "ch_on", power);
        bass.setButtonText ("Bass");
        bass.setColour (juce::ToggleButton::tickColourId, Theme::pink);
        bassAtt = std::make_unique<APVTS::ButtonAttachment> (s, "ch_bass", bass);
        strum = std::make_unique<Knob> (s, "ch_strum", "Strum", Theme::pink);
        human = std::make_unique<Knob> (s, "ch_human", "Humanize", Theme::pink);
        for (juce::Component* c : { (juce::Component*) &power, (juce::Component*) &mode, (juce::Component*) &key, (juce::Component*) &scale, (juce::Component*) &ext,
                                    (juce::Component*) &type, (juce::Component*) &voicing, (juce::Component*) &inv, (juce::Component*) &dir, (juce::Component*) &bass,
                                    (juce::Component*) strum.get(), (juce::Component*) human.get(), (juce::Component*) &generate, (juce::Component*) &play, (juce::Component*) &drag })
            addAndMakeVisible (c);
        key.setTooltip ("Key"); scale.setTooltip ("Scale"); ext.setTooltip ("Chord size"); type.setTooltip ("Chord played on every key");
        voicing.setTooltip ("Voicing"); inv.setTooltip ("Inversion"); dir.setTooltip ("Strum direction");
        mode.setTooltip ("Follow Key: every note plays the chord that fits the key. Fixed Chord: every note plays the same chord type.");

        for (int i = 0; i < 7; ++i)
        {
            auto* pad = keyPads.add (new ChordPad());
            pad->onPress = [this, i] (bool on) { padPress (i, i, on); };
            addAndMakeVisible (pad);
        }
        for (int i = 0; i < 8; ++i)
        {
            auto* pad = progPads.add (new ChordPad());
            pad->onPress = [this, i] (bool on) { const int d = proc.chordEngine.progDegrees[(size_t) i].load(); padPress (10 + i, d, on); };
            pad->onMenu = [this, i] { editProgSlot (i); };
            addAndMakeVisible (pad);
        }
        generate.setButtonText ("Generate");
        generate.getProperties().set ("gradient", true);
        generate.setColour (juce::TextButton::buttonOnColourId, Theme::pink);
        generate.setToggleState (true, juce::dontSendNotification);
        generate.setTooltip ("Make a new chord progression in this key");
        generate.onClick = [this] { generateProgression(); };
        play.setButtonText ("Play");
        play.setTooltip ("Loop the progression, one chord per bar at the song tempo");
        play.onClick = [this] { proc.chordEngine.progPlaying = ! proc.chordEngine.progPlaying.load(); };
        drag.setButtonText ("Drag MIDI");
        drag.setTooltip ("Drag the progression into your DAW as a MIDI clip (or click to save a .mid file)");
        drag.makeFile = [this] { return writeMidi (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("PresetLab Chords.mid")); };
        drag.onPlainClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> ("Save chord progression", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("PresetLab Chords.mid"), "*.mid");
            chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
                                  [this] (const juce::FileChooser& fc) { if (fc.getResult() != juce::File()) writeMidi (fc.getResult()); });
        };
        if (proc.chordEngine.progLength.load() == 0) generateProgression();
    }

    void update()
    {
        const auto st = proc.chordSettings();
        const bool fixed = st.mode == 1;
        ext.setVisible (! fixed); type.setVisible (fixed);
        juce::String sig; sig << st.key << st.scale << st.ext << (int) fixed;
        for (int i = 0; i < 8; ++i) sig << "," << (i < proc.chordEngine.progLength.load() ? proc.chordEngine.progDegrees[(size_t) i].load() : -1);
        if (sig != lastSig)
        {
            lastSig = sig;
            for (int d = 0; d < 7; ++d) describe (*keyPads[d], d, st);
            for (int i = 0; i < 8; ++i)
            {
                auto& pad = *progPads[i];
                pad.empty = i >= proc.chordEngine.progLength.load();
                if (! pad.empty) describe (pad, proc.chordEngine.progDegrees[(size_t) i].load(), st);
                pad.repaint();
            }
        }
        const int now = proc.chordEngine.progPlaying.load() ? proc.chordEngine.progNow.load() : -1;
        for (int i = 0; i < 8; ++i) { const bool l = i == now; if (progPads[i]->lit != l) { progPads[i]->lit = l; progPads[i]->repaint(); } }
        play.setButtonText (proc.chordEngine.progPlaying.load() ? "Stop" : "Play");
        const juce::String nm = proc.chordEngine.lastRoot.load() >= 0 ? chordNameFromMask (proc.chordEngine.lastRoot.load(), proc.chordEngine.lastMask.load()) : "";
        if (nm != current) { current = nm; repaint (chordArea()); }
        const float a = st.on ? 1.f : 0.55f;
        for (juce::Component* c : { (juce::Component*) &mode, (juce::Component*) &key, (juce::Component*) &scale, (juce::Component*) &ext, (juce::Component*) &type,
                                    (juce::Component*) &voicing, (juce::Component*) &inv, (juce::Component*) &dir, (juce::Component*) &bass, (juce::Component*) strum.get(), (juce::Component*) human.get() })
            c->setAlpha (a);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        drawSectionTitle (g, { 14, 48, 70, 16 }, "In key");
        drawSectionTitle (g, { 14, 64, 70, 16 }, scaleShort(), Theme::faint);
        drawSectionTitle (g, { 14, 100, 90, 16 }, "Progression");
        auto ca = chordArea();
        g.setColour (Theme::panel2); g.fillRoundedRectangle (ca.toFloat(), 8.f);
        g.setColour (current.isEmpty() ? Theme::faint : Theme::text);
        g.setFont (Theme::font (current.isEmpty() ? 12.f : 22.f, true));
        g.drawText (current.isEmpty() ? "play a chord" : current, ca, juce::Justification::centred);
    }

    void resized() override
    {
        int x = 12; const int y = 8, h = 28;
        auto put = [&] (juce::Component& c, int w) { c.setBounds (x, y, w, h); x += w + 6; };
        put (power, 96); put (mode, 104); put (key, 56); put (scale, 112);
        ext.setBounds (x, y, 88, h); type.setBounds (x, y, 88, h); x += 94;
        put (voicing, 86); put (inv, 78); put (dir, 84); put (bass, 66);
        const int right = getWidth() - 12;
        strum->setBounds (right - 136, 42, 66, 100); human->setBounds (right - 68, 42, 66, 100);
        const int px = 92, pw = right - 148 - px;
        const int kw = (pw - 6 * 6) / 7;
        for (int i = 0; i < 7; ++i) keyPads[i]->setBounds (px + i * (kw + 6), 44, kw, 46);
        generate.setBounds (px, 98, 84, 46);
        drag.setBounds (px + pw - 84, 98, 84, 46);
        play.setBounds (px + pw - 84 - 6 - 58, 98, 58, 46);
        const int gx = px + 90, gw = pw - 90 - 84 - 6 - 58 - 6;
        const int qw = (gw - 7 * 5) / 8;
        for (int i = 0; i < 8; ++i) progPads[i]->setBounds (gx + i * (qw + 5), 98, qw, 46);
    }

private:
    juce::Rectangle<int> chordArea() const { return { getWidth() - 136 - 12 + 6, 8, 130, 28 }; }
    juce::String scaleShort() const { return pl::keyNames()[(int) proc.pv ("ch_key")] + " " + pl::scaleNames()[(int) proc.pv ("ch_scale")]; }

    static juce::String chordNameFromMask (int root, int mask)
    {
        std::vector<int> iv;
        for (int i = 0; i < 12; ++i) if (mask & (1 << i)) iv.push_back (i);
        return chords::name (root, iv);
    }

    void describe (ChordPad& pad, int d, const chords::Settings& st)
    {
        d = juce::jlimit (0, 6, d);
        auto iv = chords::diatonic (d, st.scale, st.ext);
        const int rootPc = (st.key + chords::scale (st.scale)[(size_t) d]) % 12;
        pad.name = chords::name (rootPc, iv);
        pad.roman = chords::roman (d, iv);
        static const juce::uint32 fn[] { 0xff8b5cff, 0xff38d6c4, 0xff8b5cff, 0xff38d6c4, 0xffff7a3d, 0xff8b5cff, 0xffff7a3d };
        pad.colour = juce::Colour (fn[d]);
        pad.repaint();
    }

    void padPress (int id, int degree, bool on)
    {
        const auto st = proc.chordSettings();
        proc.chordEngine.pushPad ({ on, id, ChordEngine::padRoot (degree, st), degree, 0.85f });
    }

    void generateProgression()
    {
        static const std::vector<std::vector<int>> major {
            { 0, 4, 5, 3 }, { 5, 3, 0, 4 }, { 0, 5, 3, 4 }, { 1, 4, 0, 0 }, { 0, 3, 4, 3 }, { 0, 2, 3, 4 }, { 3, 0, 4, 5 },
            { 0, 4, 5, 2, 3, 0, 3, 4 }, { 0, 3, 5, 4 }, { 0, 5, 1, 4 }, { 3, 4, 2, 5 }, { 0, 1, 3, 4 }, { 0, 3, 0, 4 }, { 5, 4, 3, 4 } };
        static const std::vector<std::vector<int>> minor {
            { 0, 5, 2, 6 }, { 0, 3, 4, 0 }, { 0, 6, 5, 6 }, { 0, 5, 3, 4 }, { 0, 3, 6, 2 }, { 5, 6, 0, 0 }, { 0, 2, 6, 3 }, { 0, 6, 5, 4 },
            { 0, 3, 5, 4 }, { 0, 5, 2, 6, 0, 5, 3, 4 } };
        const int sc = (int) proc.pv ("ch_scale");
        const bool isMinor = sc == 1 || sc == 2 || sc == 3 || sc == 6;
        const auto& list = isMinor ? minor : major;
        int pick;
        do pick = rng.nextInt ((int) list.size()); while (list.size() > 1 && pick == lastPick);
        lastPick = pick;
        const auto& prog = list[(size_t) pick];
        proc.chordEngine.progLength = 0;
        for (size_t i = 0; i < prog.size(); ++i) proc.chordEngine.progDegrees[i] = prog[i];
        proc.chordEngine.progLength = (int) prog.size();
        lastSig.clear();
    }

    void editProgSlot (int i)
    {
        const auto st = proc.chordSettings();
        juce::PopupMenu m;
        for (int d = 0; d < 7; ++d)
        {
            auto iv = chords::diatonic (d, st.scale, st.ext);
            m.addItem (d + 1, chords::roman (d, iv) + "   " + chords::name ((st.key + chords::scale (st.scale)[(size_t) d]) % 12, iv));
        }
        const int len = proc.chordEngine.progLength.load();
        if (i < len) { m.addSeparator(); m.addItem (100, "Remove"); }
        m.addSeparator(); m.addItem (101, "Clear all");
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (progPads[i]), [this, i] (int r)
        {
            auto& ce = proc.chordEngine;
            int len2 = ce.progLength.load();
            if (r >= 1 && r <= 7)
            {
                const int slot = std::min (i, len2);
                ce.progDegrees[(size_t) slot] = r - 1;
                if (slot >= len2) ce.progLength = std::min (8, len2 + 1);
            }
            else if (r == 100 && i < len2)
            {
                for (int k = i; k < len2 - 1; ++k) ce.progDegrees[(size_t) k] = ce.progDegrees[(size_t) k + 1].load();
                ce.progLength = len2 - 1;
            }
            else if (r == 101) { ce.progLength = 0; ce.progPlaying = false; }
            lastSig.clear();
        });
    }

    juce::File writeMidi (const juce::File& f)
    {
        const auto st = proc.chordSettings();
        juce::MidiMessageSequence seq;
        const int ppq = 960, bar = ppq * 4;
        seq.addEvent (juce::MidiMessage::tempoMetaEvent (500000), 0);
        seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
        const int len = proc.chordEngine.progLength.load();
        for (int i = 0; i < len; ++i)
        {
            const int d = proc.chordEngine.progDegrees[(size_t) i].load();
            const auto notes = chords::forNote (ChordEngine::padRoot (d, st), st, true, d);
            for (int n : notes)
            {
                seq.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 96), (double) (i * bar));
                seq.addEvent (juce::MidiMessage::noteOff (1, n), (double) ((i + 1) * bar - 10));
            }
        }
        seq.updateMatchedPairs();
        juce::MidiFile mf;
        mf.setTicksPerQuarterNote (ppq);
        mf.addTrack (seq);
        f.deleteFile();
        juce::FileOutputStream out (f);
        if (out.openedOk()) mf.writeTo (out);
        return f;
    }

    PresetLabProcessor& proc;
    juce::ToggleButton power, bass;
    std::unique_ptr<APVTS::ButtonAttachment> powerAtt, bassAtt;
    ChoiceBox mode, key, scale, ext, type, voicing, inv, dir;
    std::unique_ptr<Knob> strum, human;
    juce::OwnedArray<ChordPad> keyPads, progPads;
    juce::TextButton generate, play;
    DragMidiButton drag;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String lastSig, current;
    juce::Random rng;
    int lastPick = -1;
};

// ================================================================================================
class Browser : public juce::Component, private juce::ListBoxModel
{
public:
    struct Entry { juce::String name, category; int factory = -1; juce::File file; };
    std::function<void()> onLoaded;

    explicit Browser (PresetLabProcessor& p) : proc (p)
    {
        search.setTextToShowWhenEmpty ("Search presets", Theme::faint);
        search.setFont (Theme::font (13.f));
        search.setIndents (10, 7);
        search.onTextChange = [this] { rebuild(); };
        addAndMakeVisible (search);
        cats.flow = true; cats.rowH = 24; cats.fontSize = 11.5f;
        cats.items.push_back ({ "All", Theme::text });
        for (auto* c : presets::categories()) cats.items.push_back ({ c, categoryColour (c) });
        cats.items.push_back ({ "User", Theme::green });
        cats.selected = 0;
        cats.onPick = [this] (int i) { cats.selected = i; cats.repaint(); rebuild(); };
        addAndMakeVisible (cats);
        list.setModel (this);
        list.setRowHeight (28);
        list.setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (list);
        save.setButtonText ("Save"); folder.setButtonText ("Open Folder");
        save.onClick = [this] { askSave(); };
        folder.onClick = [] { auto f = PresetLabProcessor::userPresetFolder(); f.createDirectory(); f.startAsProcess(); };
        addAndMakeVisible (save); addAndMakeVisible (folder);
        rebuild();
    }

    static juce::Colour categoryColour (const juce::String& c)
    {
        static const std::pair<const char*, juce::uint32> m[] {
            { "Guitar", 0xffff7a3d }, { "Bass", 0xffffb02e }, { "Keys", 0xff8b5cff }, { "Organ", 0xffc0884a }, { "Synth Lead", 0xffff3d8b },
            { "Synth Bass", 0xffd94dff }, { "Pads", 0xff38d6c4 }, { "Strings & Brass", 0xffe8c15a }, { "Bells & Mallets", 0xff2ec4b6 },
            { "Wind", 0xff8fd14f }, { "Drums", 0xffff4f5e }, { "FX & Textures", 0xffb06cff }, { "Live Guitar", 0xffff9f43 },
            { "Live Bass", 0xfff7c948 }, { "Live Vocal & Mic", 0xff5ad1ff }, { "User", 0xff3ddc84 } };
        for (auto& e : m) if (c == e.first) return juce::Colour (e.second);
        return Theme::dim;
    }

    void rebuild()
    {
        all.clear();
        const auto& f = presets::factory();
        for (int i = 0; i < (int) f.size(); ++i) all.push_back ({ f[(size_t) i].name, f[(size_t) i].category, i, {} });
        for (auto& file : PresetLabProcessor::userPresetFolder().findChildFiles (juce::File::findFiles, false, "*.plpreset"))
            all.push_back ({ file.getFileNameWithoutExtension(), "User", -1, file });
        const juce::String cat = cats.selected > 0 ? cats.items[(size_t) cats.selected].name : juce::String();
        const auto q = search.getText().trim();
        shown.clear();
        for (auto& e : all)
            if ((cat.isEmpty() || e.category == cat) && (q.isEmpty() || e.name.containsIgnoreCase (q) || e.category.containsIgnoreCase (q)))
                shown.push_back (e);
        list.updateContent();
        syncSelection();
        repaint();
    }

    void syncSelection()
    {
        for (int i = 0; i < (int) shown.size(); ++i)
            if (isCurrent (shown[(size_t) i]))
            {
                list.selectRow (i, true, true);
                return;
            }
        list.deselectAllRows();
    }

    void step (int dir)
    {
        if (shown.empty()) return;
        int cur = -1;
        for (int i = 0; i < (int) shown.size(); ++i) if (isCurrent (shown[(size_t) i])) cur = i;
        const int n = (int) shown.size();
        load (shown[(size_t) (((cur < 0 ? (dir > 0 ? -1 : 0) : cur) + dir + n) % n)]);
    }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        g.setColour (Theme::text); g.setFont (Theme::font (13.f, true));
        g.drawText ("PRESETS", 14, 10, 120, 20, juce::Justification::centredLeft);
        g.setColour (Theme::faint); g.setFont (Theme::font (11.5f));
        g.drawText (juce::String ((int) shown.size()) + " shown", getWidth() - 110, 10, 96, 20, juce::Justification::centredRight);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        r.removeFromTop (26);
        search.setBounds (r.removeFromTop (30));
        r.removeFromTop (8);
        cats.setBounds (r.getX(), r.getY(), r.getWidth(), cats.heightFor (r.getWidth()));
        r.removeFromTop (cats.getHeight() + 10);
        auto bottom = r.removeFromBottom (30);
        save.setBounds (bottom.removeFromLeft (bottom.getWidth() / 2 - 3));
        bottom.removeFromLeft (6);
        folder.setBounds (bottom);
        r.removeFromBottom (8);
        list.setBounds (r);
    }

    void askSave()
    {
        auto* w = new juce::AlertWindow ("Save preset", "Name your sound. It is saved in Documents/PresetLab Presets.", juce::MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", proc.presetName);
        w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
        w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
        w->enterModalState (true, juce::ModalCallbackFunction::create ([this, w] (int r)
        {
            const auto name = w->getTextEditorContents ("name").trim();
            if (r == 1 && name.isNotEmpty()) { proc.saveUserPreset (name); rebuild(); }
        }), true);
    }

private:
    bool isCurrent (const Entry& e) const
    {
        return e.factory >= 0 ? e.factory == proc.presetIndex : (proc.presetIndex < 0 && proc.presetCategory == "User" && e.name == proc.presetName);
    }
    void load (const Entry& e)
    {
        if (e.factory >= 0) proc.loadFactoryPreset (e.factory);
        else proc.loadUserPreset (e.file);
        syncSelection();
        if (onLoaded) onLoaded();
    }

    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool sel) override
    {
        if (row < 0 || row >= (int) shown.size()) return;
        const auto& e = shown[(size_t) row];
        auto r = juce::Rectangle<float> (0.f, 1.f, (float) w, (float) h - 2.f);
        if (sel) { g.setColour (Theme::pink.withAlpha (0.18f)); g.fillRoundedRectangle (r, 7.f); g.setColour (Theme::pink.withAlpha (0.6f)); g.drawRoundedRectangle (r.reduced (0.5f), 7.f, 1.f); }
        g.setColour (categoryColour (e.category));
        g.fillEllipse (juce::Rectangle<float> (7.f, 7.f).withCentre ({ 12.f, r.getCentreY() }));
        g.setColour (sel ? juce::Colours::white : Theme::text.withAlpha (0.88f));
        g.setFont (Theme::font (13.f, sel));
        g.drawText (e.name, juce::Rectangle<int> (24, 0, w - (cats.selected == 0 ? 112 : 30), h), juce::Justification::centredLeft, true);
        if (cats.selected == 0)
        {
            g.setColour (Theme::faint); g.setFont (Theme::font (10.5f));
            g.drawText (e.category, juce::Rectangle<int> (w - 88, 0, 82, h), juce::Justification::centredRight, true);
        }
    }
    void listBoxItemClicked (int row, const juce::MouseEvent&) override { if (row >= 0 && row < (int) shown.size()) load (shown[(size_t) row]); }

    PresetLabProcessor& proc;
    juce::TextEditor search;
    PillGrid cats;
    juce::ListBox list;
    juce::TextButton save, folder;
    std::vector<Entry> all, shown;
};
