#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "Look.h"

using APVTS = juce::AudioProcessorValueTreeState;

// A labelled rotary knob attached to a parameter. Double-click resets it.
class Knob : public juce::Component
{
public:
    Knob (APVTS& s, const juce::String& id, const juce::String& name, juce::Colour c, bool bipolar = false)
        : slider (juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow)
    {
        slider.setRotaryParameters (Theme::kPi * 1.25f, Theme::kPi * 2.75f, true);
        slider.setColour (juce::Slider::rotarySliderFillColourId, c);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 16);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxTextColourId, Theme::text.withAlpha (0.85f));
        slider.getProperties().set ("bipolar", bipolar);
        slider.setMouseDragSensitivity (220);
        addAndMakeVisible (slider);
        label.setText (name, juce::dontSendNotification);
        label.setFont (Theme::font (11.f, true));
        label.setColour (juce::Label::textColourId, Theme::text.withAlpha (0.8f));
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
        att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
        if (auto* p = s.getParameter (id))
            slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    }

    void setLabel (const juce::String& t) { if (label.getText() != t) label.setText (t, juce::dontSendNotification); }
    void setKnobColour (juce::Colour c) { slider.setColour (juce::Slider::rotarySliderFillColourId, c); }
    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (15));
        slider.setBounds (r);
    }

    juce::Slider slider;
    juce::Label label;

private:
    std::unique_ptr<APVTS::SliderAttachment> att;
};

// A combo box filled with a choice parameter's options and attached to it.
class ChoiceBox : public juce::ComboBox
{
public:
    ChoiceBox (APVTS& s, const juce::String& id)
    {
        if (auto* c = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id)))
            addItemList (c->choices, 1);
        att = std::make_unique<APVTS::ComboBoxAttachment> (s, id, *this);
    }
private:
    std::unique_ptr<APVTS::ComboBoxAttachment> att;
};

// Illuminated power button; with the "foot" property it is drawn as a stomp box footswitch.
class PowerButton : public juce::Button
{
public:
    PowerButton() : juce::Button ("power") { setClickingTogglesState (true); }
    juce::Colour colour = Theme::accent;
    bool foot = false;

    void paintButton (juce::Graphics& g, bool hl, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (2.f);
        const float d = std::min (r.getWidth(), r.getHeight());
        auto k = juce::Rectangle<float> (d, d).withCentre (r.getCentre());
        const bool on = getToggleState();
        if (foot)
        {
            g.setColour (juce::Colours::black.withAlpha (0.45f));
            g.fillEllipse (k.translated (0.f, 3.f));
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe9ecf1), k.getX(), k.getY(), juce::Colour (0xff70798a), k.getRight(), k.getBottom(), false));
            g.fillEllipse (k);
            auto in = k.reduced (d * 0.16f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8f98a8), in.getX(), in.getY(), juce::Colour (0xffd9dde4), in.getRight(), in.getBottom(), false));
            g.fillEllipse (down ? in.reduced (1.f) : in);
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.drawEllipse (k, 1.f);
            return;
        }
        if (on)
            for (int i = 3; i >= 1; --i) { g.setColour (colour.withAlpha (0.09f * (float) i)); g.fillEllipse (k.expanded ((float) (4 - i) * 2.f)); }
        g.setColour (Theme::panel3.brighter (hl ? 0.1f : 0.f));
        g.fillEllipse (k);
        g.setColour (on ? colour : Theme::dim);
        const auto c = k.getCentre();
        const float pr = d * 0.26f;
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, pr, pr, 0.f, 0.65f, 2.f * Theme::kPi - 0.65f, true);
        g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.drawLine (c.x, c.y - pr - 1.5f, c.x, c.y - pr * 0.1f, 1.8f);
        g.setColour (on ? colour.withAlpha (0.6f) : Theme::border);
        g.drawEllipse (k, 1.f);
    }
};

// A grid (or flowing row) of selectable pills.
class PillGrid : public juce::Component
{
public:
    struct Item { juce::String name; juce::Colour colour; };
    std::vector<Item> items;
    int selected = -1, cols = 3, rowH = 30;
    bool flow = false;
    float fontSize = 12.5f;
    std::function<void (int)> onPick;

    juce::Rectangle<float> cell (int i) const
    {
        if (flow) return i < (int) flowRects.size() ? flowRects[(size_t) i] : juce::Rectangle<float>();
        const float gap = 6.f, w = ((float) getWidth() - gap * (float) (cols - 1)) / (float) cols;
        return { (float) (i % cols) * (w + gap), (float) (i / cols) * ((float) rowH + gap), w, (float) rowH };
    }
    int heightFor (int width)
    {
        if (! flow) return ((int) items.size() + cols - 1) / cols * (rowH + 6) - 6;
        layoutFlow ((float) width);
        return flowRects.empty() ? 0 : (int) flowRects.back().getBottom();
    }
    void resized() override { if (flow) layoutFlow ((float) getWidth()); }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < (int) items.size(); ++i)
        {
            auto r = cell (i).reduced (0.5f);
            const bool sel = i == selected, hov = i == hover;
            const auto c = items[(size_t) i].colour;
            g.setColour (sel ? c.withAlpha (0.9f) : Theme::panel2.brighter (hov ? 0.07f : 0.f));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f > 10.f ? 9.f : r.getHeight() * 0.5f);
            g.setColour (sel ? c.brighter (0.4f) : hov ? c.withAlpha (0.7f) : Theme::border);
            g.drawRoundedRectangle (r, 9.f, 1.f);
            if (! sel)
            {
                g.setColour (c);
                g.fillEllipse (juce::Rectangle<float> (6.f, 6.f).withCentre ({ r.getX() + 12.f, r.getCentreY() }));
            }
            g.setColour (sel ? (c.getPerceivedBrightness() > 0.65f ? juce::Colours::black : juce::Colours::white) : Theme::text.withAlpha (0.9f));
            g.setFont (Theme::font (fontSize, sel));
            g.drawFittedText (items[(size_t) i].name, r.toNearestInt().withTrimmedLeft (sel ? 6 : 22).withTrimmedRight (6),
                              sel ? juce::Justification::centred : juce::Justification::centredLeft, 1, 0.8f);
        }
    }
    void mouseMove (const juce::MouseEvent& e) override { const int h = hit (e.position); if (h != hover) { hover = h; repaint(); } }
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int h = hit (e.position);
        if (h >= 0 && onPick) onPick (h);
    }

private:
    int hit (juce::Point<float> p) const { for (int i = 0; i < (int) items.size(); ++i) if (cell (i).contains (p)) return i; return -1; }
    void layoutFlow (float width)
    {
        flowRects.clear();
        float x = 0.f, y = 0.f;
        for (auto& it : items)
        {
            const float w = juce::GlyphArrangement::getStringWidth (Theme::font (fontSize, true), it.name) + 34.f;
            if (x > 0.f && x + w > width) { x = 0.f; y += (float) rowH + 6.f; }
            flowRects.push_back ({ x, y, w, (float) rowH });
            x += w + 6.f;
        }
    }
    std::vector<juce::Rectangle<float>> flowRects;
    int hover = -1;
};

// level meter fed by a getter (0..1 linear)
class Meter : public juce::Component
{
public:
    std::function<float()> get;
    bool horizontal = false;
    void update()
    {
        const float v = get ? get() : 0.f;
        const float db = juce::jlimit (-60.f, 6.f, juce::Decibels::gainToDecibels (v, -60.f));
        const float n = (db + 60.f) / 66.f;
        if (std::abs (n - level) > 0.004f) { level = n; repaint(); }
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (Theme::panel3);
        g.fillRoundedRectangle (r, 2.f);
        auto f = horizontal ? r.withWidth (r.getWidth() * level) : r.withTrimmedTop (r.getHeight() * (1.f - level));
        juce::ColourGradient cg (Theme::green, horizontal ? r.getX() : 0.f, horizontal ? 0.f : r.getBottom(),
                                 Theme::red, horizontal ? r.getRight() : 0.f, horizontal ? 0.f : r.getY(), false);
        cg.addColour (0.75, Theme::amber);
        g.setGradientFill (cg);
        g.fillRoundedRectangle (f, 2.f);
    }
private:
    float level = 0.f;
};

inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 12.f)
{
    g.setColour (Theme::panel);
    g.fillRoundedRectangle (r, radius);
    g.setColour (Theme::border.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.f);
}

inline void drawSectionTitle (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& t, juce::Colour c = Theme::dim)
{
    g.setColour (c);
    g.setFont (Theme::font (11.f, true));
    g.drawText (t.toUpperCase(), r, juce::Justification::centredLeft);
}
