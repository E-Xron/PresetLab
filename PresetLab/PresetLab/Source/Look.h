#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>
#include "Params.h"

namespace Theme
{
    constexpr float kPi = 3.14159265358979f;
    const juce::Colour bg      { 0xff0c0e13 };
    const juce::Colour panel   { 0xff13161d };
    const juce::Colour panel2  { 0xff1b1f28 };
    const juce::Colour panel3  { 0xff232835 };
    const juce::Colour border  { 0xff2a303d };
    const juce::Colour text    { 0xffeef1f6 };
    const juce::Colour dim     { 0xff8b93a4 };
    const juce::Colour faint   { 0xff5a6273 };
    const juce::Colour orange  { 0xffff7a3d };
    const juce::Colour pink    { 0xffff3d8b };
    const juce::Colour violet  { 0xff8b5cff };
    const juce::Colour cyan    { 0xff38d6c4 };
    const juce::Colour amber   { 0xffffb02e };
    const juce::Colour green   { 0xff3ddc84 };
    const juce::Colour red     { 0xffff4f5e };
    const juce::Colour accent  = pink;

    inline juce::ColourGradient brand (juce::Rectangle<float> r)
    {
        juce::ColourGradient g (orange, r.getX(), r.getY(), violet, r.getRight(), r.getBottom(), false);
        g.addColour (0.5, pink);
        return g;
    }
    inline juce::Font font (float size, bool bold = false) { return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain)); }
}

// ------------------------------------------------------------------------------------------------
// The PresetLab logo: a squircle in the brand gradient holding a "P" whose bowl is a knob.
inline void drawLogoIcon (juce::Graphics& g, juce::Rectangle<float> r)
{
    const float s = std::min (r.getWidth(), r.getHeight());
    r = r.withSizeKeepingCentre (s, s);
    g.setGradientFill (Theme::brand (r));
    g.fillRoundedRectangle (r, s * 0.26f);
    g.setColour (juce::Colours::white.withAlpha (0.16f));
    g.drawRoundedRectangle (r.reduced (s * 0.02f), s * 0.24f, s * 0.02f);

    const float x0 = r.getX(), y0 = r.getY();
    auto P = [&] (float fx, float fy) { return juce::Point<float> (x0 + fx * s, y0 + fy * s); };
    // stem
    juce::Path stem;
    stem.addRoundedRectangle (P (0.27f, 0.22f).x, P (0.27f, 0.22f).y, s * 0.12f, s * 0.58f, s * 0.06f);
    g.setColour (juce::Colours::white);
    g.fillPath (stem);
    // knob bowl: 300 degree ring with a gap where it meets the stem, and a pointer dot
    const auto c = P (0.52f, 0.41f);
    const float rr = s * 0.19f;
    juce::Path ring;
    ring.addCentredArc (c.x, c.y, rr, rr, 0.f, -Theme::kPi * 0.92f, Theme::kPi * 0.62f, true);
    g.strokePath (ring, juce::PathStrokeType (s * 0.11f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const float pa = Theme::kPi * 0.25f;
    g.fillEllipse (juce::Rectangle<float> (s * 0.09f, s * 0.09f).withCentre ({ c.x + rr * 0.42f * std::sin (pa), c.y - rr * 0.42f * std::cos (pa) }));
    // three sound waves
    g.setColour (juce::Colours::white.withAlpha (0.85f));
    for (int i = 0; i < 3; ++i)
    {
        juce::Path w;
        const float wr = s * (0.1f + 0.075f * (float) i);
        w.addCentredArc (P (0.6f, 0.7f).x, P (0.6f, 0.7f).y, wr, wr, 0.f, Theme::kPi * 0.62f, Theme::kPi * 0.88f, true);
        g.setColour (juce::Colours::white.withAlpha (0.9f - 0.22f * (float) i));
        g.strokePath (w, juce::PathStrokeType (s * 0.045f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

inline void drawWordmark (juce::Graphics& g, juce::Rectangle<float> r, float size)
{
    auto f = Theme::font (size, true);
    const float w1 = juce::GlyphArrangement::getStringWidth (f, "Preset");
    g.setFont (f);
    g.setColour (Theme::text);
    g.drawText ("Preset", r.removeFromLeft (w1 + 1.f), juce::Justification::centredLeft, false);
    const float w2 = juce::GlyphArrangement::getStringWidth (f, "Lab");
    auto lr = r.removeFromLeft (w2 + 2.f);
    g.setGradientFill (juce::ColourGradient (Theme::orange, lr.getX(), 0.f, Theme::violet, lr.getRight(), 0.f, false));
    g.drawText ("Lab", lr, juce::Justification::centredLeft, false);
}

// ------------------------------------------------------------------------------------------------
// instrument icons
inline void drawInstIcon (juce::Graphics& g, int inst, juce::Rectangle<float> r, juce::Colour c)
{
    const float s = std::min (r.getWidth(), r.getHeight());
    r = r.withSizeKeepingCentre (s, s);
    auto P = [&] (float fx, float fy) { return juce::Point<float> (r.getX() + fx * s, r.getY() + fy * s); };
    const float lw = std::max (1.4f, s * 0.06f);
    juce::PathStrokeType st (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    g.setColour (c);
    juce::Path p;
    switch (inst)
    {
        case pl::AcousticGtr:
            p.addEllipse (P (0.18f, 0.48f).x, P (0.18f, 0.48f).y, s * 0.42f, s * 0.42f);
            p.addEllipse (P (0.26f, 0.28f).x, P (0.26f, 0.28f).y, s * 0.3f, s * 0.3f);
            g.strokePath (p, st);
            g.fillEllipse (juce::Rectangle<float> (s * 0.12f, s * 0.12f).withCentre (P (0.39f, 0.66f)));
            g.drawLine ({ P (0.45f, 0.55f), P (0.86f, 0.12f) }, lw * 1.3f);
            break;
        case pl::ElectricGtr: case pl::BassGtr:
        {
            const bool bass = inst == pl::BassGtr;
            p.startNewSubPath (P (0.2f, 0.62f));
            p.cubicTo (P (0.08f, 0.82f), P (0.3f, 0.95f), P (0.45f, 0.8f));
            p.cubicTo (P (0.55f, 0.7f), P (0.5f, 0.6f), P (0.6f, 0.52f));
            p.cubicTo (P (0.5f, 0.42f), P (0.4f, 0.5f), P (0.36f, 0.38f));
            p.cubicTo (P (0.25f, 0.45f), P (0.3f, 0.52f), P (0.2f, 0.62f));
            g.strokePath (p, st);
            g.drawLine ({ P (0.42f, 0.62f), bass ? P (0.92f, 0.06f) : P (0.86f, 0.14f) }, lw * 1.3f);
            g.drawLine ({ P (0.27f, 0.7f), P (0.37f, 0.6f) }, lw);
            if (! bass) g.drawLine ({ P (0.32f, 0.76f), P (0.42f, 0.66f) }, lw);
            break;
        }
        case pl::SynthBass:
            p.startNewSubPath (P (0.1f, 0.7f));
            for (int i = 0; i < 2; ++i) { p.lineTo (P (0.1f + 0.4f * (float) (i + 1), 0.3f)); p.lineTo (P (0.1f + 0.4f * (float) (i + 1), 0.7f)); }
            g.strokePath (p, st);
            g.drawLine ({ P (0.1f, 0.86f), P (0.9f, 0.86f) }, lw * 1.6f);
            break;
        case pl::Piano: case pl::EPiano:
        {
            auto k = juce::Rectangle<float> (P (0.12f, inst == pl::EPiano ? 0.42f : 0.28f), P (0.88f, 0.84f));
            g.drawRoundedRectangle (k, s * 0.04f, lw);
            for (int i = 1; i < 5; ++i) g.drawLine (k.getX() + k.getWidth() * (float) i / 5.f, k.getY(), k.getX() + k.getWidth() * (float) i / 5.f, k.getBottom(), lw * 0.7f);
            for (int i : { 1, 2, 4 }) g.fillRect (juce::Rectangle<float> (k.getWidth() * 0.09f, k.getHeight() * 0.55f).withCentre ({ k.getX() + k.getWidth() * (float) i / 5.f, k.getY() + k.getHeight() * 0.28f }));
            if (inst == pl::EPiano)
            {
                juce::Path w; w.startNewSubPath (P (0.12f, 0.26f));
                for (int i = 1; i <= 24; ++i) w.lineTo (P (0.12f + 0.76f * (float) i / 24.f, 0.26f - 0.1f * std::sin ((float) i / 24.f * Theme::kPi * 4.f)));
                g.strokePath (w, juce::PathStrokeType (lw * 0.8f));
            }
            break;
        }
        case pl::Organ:
        {
            const float h[] { 0.6f, 0.5f, 0.7f, 0.3f, 0.45f, 0.2f, 0.35f };
            for (int i = 0; i < 7; ++i)
            {
                const float x = 0.16f + 0.115f * (float) i;
                g.fillRoundedRectangle (juce::Rectangle<float> (P (x, 0.18f), P (x + 0.06f, 0.18f + h[i])), s * 0.02f);
            }
            g.drawLine ({ P (0.12f, 0.86f), P (0.88f, 0.86f) }, lw);
            break;
        }
        case pl::Synth:
            p.startNewSubPath (P (0.08f, 0.66f));
            for (int i = 0; i < 3; ++i) { p.lineTo (P (0.08f + 0.28f * (float) (i + 1), 0.34f)); p.lineTo (P (0.08f + 0.28f * (float) (i + 1), 0.66f)); }
            g.strokePath (p, st);
            break;
        case pl::Pad:
            for (int k = 0; k < 3; ++k)
            {
                juce::Path w; w.startNewSubPath (P (0.08f, 0.5f));
                for (int i = 1; i <= 40; ++i) w.lineTo (P (0.08f + 0.84f * (float) i / 40.f, 0.5f - (0.22f - 0.06f * (float) k) * std::sin ((float) i / 40.f * Theme::kPi * 2.f + (float) k * 0.7f)));
                g.setColour (c.withAlpha (1.f - 0.3f * (float) k));
                g.strokePath (w, juce::PathStrokeType (lw * (1.f - 0.2f * (float) k), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            break;
        case pl::Strings:
            p.startNewSubPath (P (0.38f, 0.14f));
            p.cubicTo (P (0.22f, 0.2f), P (0.32f, 0.42f), P (0.26f, 0.5f));
            p.cubicTo (P (0.18f, 0.62f), P (0.24f, 0.86f), P (0.44f, 0.88f));
            p.lineTo (P (0.56f, 0.88f));
            p.cubicTo (P (0.76f, 0.86f), P (0.82f, 0.62f), P (0.74f, 0.5f));
            p.cubicTo (P (0.68f, 0.42f), P (0.78f, 0.2f), P (0.62f, 0.14f));
            p.closeSubPath();
            g.strokePath (p, st);
            g.drawLine ({ P (0.08f, 0.82f), P (0.92f, 0.3f) }, lw);
            break;
        case pl::Brass:
            p.startNewSubPath (P (0.1f, 0.48f)); p.lineTo (P (0.6f, 0.48f));
            p.lineTo (P (0.9f, 0.3f)); p.lineTo (P (0.9f, 0.7f)); p.lineTo (P (0.6f, 0.56f)); p.lineTo (P (0.1f, 0.56f));
            g.strokePath (p, st);
            for (int i = 0; i < 3; ++i) g.drawLine ({ P (0.3f + 0.08f * (float) i, 0.48f), P (0.3f + 0.08f * (float) i, 0.34f) }, lw);
            break;
        case pl::Bell:
            p.startNewSubPath (P (0.18f, 0.74f));
            p.cubicTo (P (0.3f, 0.66f), P (0.26f, 0.2f), P (0.5f, 0.18f));
            p.cubicTo (P (0.74f, 0.2f), P (0.7f, 0.66f), P (0.82f, 0.74f));
            p.closeSubPath();
            g.strokePath (p, st);
            g.fillEllipse (juce::Rectangle<float> (s * 0.12f, s * 0.12f).withCentre (P (0.5f, 0.82f)));
            break;
        case pl::Mallets:
            for (int i = 0; i < 5; ++i)
            {
                const float h = 0.62f - 0.08f * (float) i, x = 0.12f + 0.14f * (float) i;
                g.fillRoundedRectangle (juce::Rectangle<float> (P (x, 0.5f - h * 0.5f), P (x + 0.1f, 0.5f + h * 0.5f)), s * 0.02f);
            }
            g.drawLine ({ P (0.7f, 0.9f), P (0.92f, 0.5f) }, lw);
            g.fillEllipse (juce::Rectangle<float> (s * 0.12f, s * 0.12f).withCentre (P (0.92f, 0.46f)));
            break;
        case pl::Flute:
            g.drawRoundedRectangle (juce::Rectangle<float> (P (0.06f, 0.44f), P (0.94f, 0.56f)), s * 0.05f, lw);
            for (int i = 0; i < 5; ++i) g.fillEllipse (juce::Rectangle<float> (s * 0.05f, s * 0.05f).withCentre (P (0.38f + 0.1f * (float) i, 0.5f)));
            break;
        case pl::Drums:
        default:
            g.drawEllipse (juce::Rectangle<float> (P (0.14f, 0.36f), P (0.86f, 0.56f)), lw);
            g.drawLine ({ P (0.14f, 0.46f), P (0.14f, 0.76f) }, lw);
            g.drawLine ({ P (0.86f, 0.46f), P (0.86f, 0.76f) }, lw);
            p.addCentredArc (P (0.5f, 0.76f).x, P (0.5f, 0.76f).y, s * 0.36f, s * 0.1f, 0.f, Theme::kPi * 0.5f, Theme::kPi * 1.5f, true);
            g.strokePath (p, st);
            g.drawLine ({ P (0.3f, 0.1f), P (0.48f, 0.4f) }, lw);
            g.drawLine ({ P (0.72f, 0.1f), P (0.54f, 0.4f) }, lw);
            break;
    }
}

// ------------------------------------------------------------------------------------------------
class PLLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PLLookAndFeel()
    {
        using namespace Theme;
        setColour (juce::ResizableWindow::backgroundColourId, bg);
        setColour (juce::TextButton::buttonColourId, panel2);
        setColour (juce::TextButton::textColourOffId, text);
        setColour (juce::TextButton::textColourOnId, juce::Colours::white);
        setColour (juce::TextButton::buttonOnColourId, accent);
        setColour (juce::ComboBox::textColourId, text);
        setColour (juce::ComboBox::backgroundColourId, panel2);
        setColour (juce::ComboBox::outlineColourId, border);
        setColour (juce::ComboBox::arrowColourId, dim);
        setColour (juce::PopupMenu::backgroundColourId, panel2);
        setColour (juce::PopupMenu::textColourId, text);
        setColour (juce::PopupMenu::headerTextColourId, dim);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, accent.withAlpha (0.85f));
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::Label::textColourId, text);
        setColour (juce::Slider::textBoxTextColourId, text.withAlpha (0.85f));
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::rotarySliderFillColourId, accent);
        setColour (juce::TextEditor::backgroundColourId, panel2);
        setColour (juce::TextEditor::textColourId, text);
        setColour (juce::TextEditor::outlineColourId, border);
        setColour (juce::TextEditor::focusedOutlineColourId, accent.withAlpha (0.7f));
        setColour (juce::TextEditor::highlightColourId, accent.withAlpha (0.35f));
        setColour (juce::CaretComponent::caretColourId, text);
        setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::ScrollBar::thumbColourId, border.brighter (0.2f));
        setColour (juce::ToggleButton::textColourId, text);
        setColour (juce::ToggleButton::tickColourId, accent);
        setColour (juce::TooltipWindow::backgroundColourId, panel3);
        setColour (juce::TooltipWindow::textColourId, text);
        setColour (juce::TooltipWindow::outlineColourId, border);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int h) override { return Theme::font (std::min (13.5f, (float) h * 0.48f), true); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return Theme::font (13.f, true); }
    juce::Font getPopupMenuFont() override { return Theme::font (14.f); }
    juce::Font getLabelFont (juce::Label& l) override { return l.getFont(); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        const float rad = b.getProperties().contains ("radius") ? (float) b.getProperties()["radius"] : 8.f;
        const bool cl = b.isConnectedOnLeft(), cr = b.isConnectedOnRight();
        juce::Path shape;
        shape.addRoundedRectangle (r.getX(), r.getY(), r.getWidth(), r.getHeight(), rad, rad, ! cl, ! cr, ! cl, ! cr);
        juce::Colour fill = on ? b.findColour (juce::TextButton::buttonOnColourId) : b.findColour (juce::TextButton::buttonColourId);
        if (hl) fill = fill.brighter (0.08f);
        if (down) fill = fill.darker (0.12f);
        if (on && b.getProperties()["gradient"])
            g.setGradientFill (Theme::brand (r));
        else
            g.setGradientFill (juce::ColourGradient (fill.brighter (0.05f), 0.f, r.getY(), fill.darker (0.06f), 0.f, r.getBottom(), false));
        g.fillPath (shape);
        g.setColour (on ? fill.brighter (0.3f).withAlpha (0.6f) : Theme::border.withAlpha (hl ? 1.f : 0.8f));
        g.strokePath (shape, juce::PathStrokeType (1.f));
    }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& cb) override
    {
        auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
        g.setColour (cb.findColour (juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle (r, 7.f);
        g.setColour (cb.isMouseOver (true) ? Theme::border.brighter (0.3f) : Theme::border);
        g.drawRoundedRectangle (r, 7.f, 1.f);
        juce::Path p;
        const float cx = (float) w - 13.f, cy = (float) h * 0.5f;
        p.startNewSubPath (cx - 4.f, cy - 2.f); p.lineTo (cx, cy + 2.f); p.lineTo (cx + 4.f, cy - 2.f);
        g.setColour (Theme::dim);
        g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 1, box.getWidth() - 26, box.getHeight() - 2);
        label.setFont (getComboBoxFont (box));
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s) override
    {
        const juce::Colour c = s.findColour (juce::Slider::rotarySliderFillColourId);
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.f);
        const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
        const auto ctr = b.getCentre();
        const float ang = a0 + pos * (a1 - a0);
        const bool en = s.isEnabled();
        const float lw = std::max (2.5f, r * 0.11f);
        const float rr = r - lw * 0.5f - 1.f;

        juce::Path track, val;
        track.addCentredArc (ctr.x, ctr.y, rr, rr, 0.f, a0, a1, true);
        const bool bip = s.getProperties()["bipolar"];
        const float from = bip ? (a0 + a1) * 0.5f : a0;
        val.addCentredArc (ctr.x, ctr.y, rr, rr, 0.f, std::min (from, ang), std::max (from, ang), true);
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.strokePath (track, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        if (en)
        {
            g.setColour (c.withAlpha (0.18f));
            g.strokePath (val, juce::PathStrokeType (lw * 2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        g.setColour (en ? c : Theme::faint);
        g.strokePath (val, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const float kr = rr - lw * 1.4f;
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillEllipse (ctr.x - kr, ctr.y - kr + 2.f, kr * 2.f, kr * 2.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff343a48), ctr.x, ctr.y - kr, juce::Colour (0xff181b22), ctr.x, ctr.y + kr, false));
        g.fillEllipse (ctr.x - kr, ctr.y - kr, kr * 2.f, kr * 2.f);
        g.setColour (juce::Colours::white.withAlpha (s.isMouseOverOrDragging() ? 0.22f : 0.12f));
        g.drawEllipse (ctr.x - kr + 0.5f, ctr.y - kr + 0.5f, kr * 2.f - 1.f, kr * 2.f - 1.f, 1.f);

        const float p0 = kr * 0.3f, p1 = kr * 0.82f;
        g.setColour (en ? juce::Colours::white : Theme::dim);
        g.drawLine (ctr.x + p0 * std::sin (ang), ctr.y - p0 * std::cos (ang), ctr.x + p1 * std::sin (ang), ctr.y - p1 * std::cos (ang), std::max (2.f, kr * 0.13f));
    }

    juce::Label* createSliderTextBox (juce::Slider& s) override
    {
        auto* l = LookAndFeel_V4::createSliderTextBox (s);
        l->setFont (Theme::font (11.5f));
        l->setColour (juce::Label::textColourId, Theme::dim);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        l->setColour (juce::TextEditor::backgroundColourId, Theme::panel2);
        return l;
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        const juce::Colour c = b.findColour (juce::ToggleButton::tickColourId);
        const bool on = b.getToggleState();
        auto r = b.getLocalBounds().toFloat();
        auto sw = r.removeFromLeft (34.f).withSizeKeepingCentre (30.f, 16.f);
        g.setColour (on ? c : Theme::panel3);
        g.fillRoundedRectangle (sw, 8.f);
        g.setColour (Theme::border);
        if (! on) g.drawRoundedRectangle (sw, 8.f, 1.f);
        const float kx = on ? sw.getRight() - 14.f : sw.getX() + 2.f;
        g.setColour (juce::Colours::white.withAlpha (on ? 1.f : 0.75f));
        g.fillEllipse (kx, sw.getY() + 2.f, 12.f, 12.f);
        g.setColour (Theme::text.withAlpha (hl ? 1.f : 0.85f));
        g.setFont (Theme::font (12.5f, true));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (4.f), juce::Justification::centredLeft);
    }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (Theme::panel2);
        g.setColour (Theme::border);
        g.drawRect (0, 0, w, h);
    }

    void drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool over, bool) override
    {
        auto t = vertical ? juce::Rectangle<int> (x + w / 2 - 2, thumbStart, 4, thumbSize) : juce::Rectangle<int> (thumbStart, y + h / 2 - 2, thumbSize, 4);
        g.setColour (Theme::border.brighter (over ? 0.5f : 0.2f));
        g.fillRoundedRectangle (t.toFloat(), 2.f);
    }

    void fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor& e) override
    {
        g.setColour (e.findColour (juce::TextEditor::backgroundColourId));
        g.fillRoundedRectangle (0.f, 0.f, (float) w, (float) h, 7.f);
    }
    void drawTextEditorOutline (juce::Graphics& g, int w, int h, juce::TextEditor& e) override
    {
        g.setColour (e.hasKeyboardFocus (true) ? Theme::accent.withAlpha (0.7f) : Theme::border);
        g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.f, (float) h - 1.f, 7.f, 1.f);
    }
};
