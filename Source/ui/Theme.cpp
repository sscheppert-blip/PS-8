#include "Theme.h"

namespace ps8::ui
{
    namespace
    {
        const juce::Identifier themeProperty { "ps8theme" };

        juce::Point<float> polar (juce::Point<float> centre, float angle, float distance)
        {
            return centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * distance;
        }
    }

    juce::String themeName (Theme theme)
    {
        switch (theme)
        {
            case Theme::controlRoom: return "Control Room";
            case Theme::matteBlack:  return "Matte Black";
            case Theme::numThemes:   break;
        }

        return {};
    }

    juce::String themeId (Theme theme)
    {
        switch (theme)
        {
            case Theme::controlRoom: return "controlRoom";
            case Theme::matteBlack:  return "matteBlack";
            case Theme::numThemes:   break;
        }

        return {};
    }

    Theme themeFromId (const juce::String& id, Theme fallback)
    {
        for (int i = 0; i < (int) Theme::numThemes; ++i)
            if (themeId ((Theme) i) == id)
                return (Theme) i;

        return fallback;
    }

    void setTheme (juce::Component& component, Theme theme)
    {
        component.getProperties().set (themeProperty, (int) theme);
    }

    Theme themeOf (const juce::Component* component)
    {
        for (auto* c = component; c != nullptr; c = c->getParentComponent())
            if (const auto* value = c->getProperties().getVarPointer (themeProperty))
                return (Theme) juce::jlimit (0, (int) Theme::numThemes - 1, (int) *value);

        return Theme::controlRoom;
    }

    //==============================================================================
    namespace matte
    {
        juce::Font font (float height, bool bold)
        {
            return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain).withKerningFactor (0.12f));
        }

        void drawLabel (juce::Graphics& g, const juce::String& text, juce::Point<float> centre, float height,
                        juce::Colour colour, juce::Justification justification, bool bold)
        {
            g.setFont (font (height, bold));
            g.setColour (colour);
            const auto h = height * 1.5f;
            juce::Rectangle<float> area (centre.x - 100.0f, centre.y - h * 0.5f, 200.0f, h);

            if (justification == juce::Justification::centredRight)
                area.setX (centre.x - 200.0f), area.setWidth (200.0f);
            else if (justification == juce::Justification::centredLeft)
                area.setX (centre.x), area.setWidth (200.0f);

            g.drawText (text.toUpperCase(), area, justification, false);
        }

        void drawChassis (juce::Graphics& g, juce::Rectangle<float> area, float cornerSize)
        {
            g.setGradientFill (juce::ColourGradient (chassisTop, 0.0f, area.getY(), chassisBottom, 0.0f, area.getBottom(), false));
            g.fillRoundedRectangle (area, cornerSize);

            // soft sheen across the panel, as on brushed and blasted aluminium
            juce::ColourGradient sheen (juce::Colours::white.withAlpha (0.055f), area.getTopLeft(),
                                        juce::Colours::white.withAlpha (0.0f), area.getBottomRight(), false);
            sheen.addColour (0.22, juce::Colours::white.withAlpha (0.0f));
            sheen.addColour (0.62, juce::Colours::white.withAlpha (0.0f));
            sheen.addColour (0.78, juce::Colours::white.withAlpha (0.03f));
            g.setGradientFill (sheen);
            g.fillRoundedRectangle (area, cornerSize);

            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawHorizontalLine ((int) area.getY() + 1, area.getX() + cornerSize, area.getRight() - cornerSize);
            g.setColour (juce::Colour (0xff050506));
            g.drawRoundedRectangle (area, cornerSize, 1.0f);
        }

        void drawSection (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title)
        {
            g.setColour (line);
            g.fillRect (area.getX(), area.getY(), area.getWidth(), 1.0f);

            if (title.isNotEmpty())
            {
                g.setFont (font (10.0f, true).withExtraKerningFactor (0.1f));
                g.setColour (inkFaint);
                g.drawText (title.toUpperCase(), juce::Rectangle<float> (area.getX(), area.getY() + 5.0f, area.getWidth(), 14.0f),
                            juce::Justification::centredLeft, false);
            }
        }

        void drawScrew (juce::Graphics& g, juce::Point<float> centre, float radius)
        {
            const auto r = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4b4e), centre - juce::Point<float> (radius, radius) * 0.3f,
                                                     juce::Colour (0xff1a1a1c), centre + juce::Point<float> (radius, radius) * 0.7f, true));
            g.fillEllipse (r);
            g.setColour (juce::Colours::black);
            g.drawEllipse (r, 1.0f);
        }

        juce::Rectangle<float> drawKey (juce::Graphics& g, juce::Rectangle<float> area, bool down, float cornerSize)
        {
            // the key's skirt and its shadow
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRoundedRectangle (area.translated (0.0f, down ? 2.0f : 4.0f).expanded (1.0f), cornerSize + 1.0f);
            g.setColour (juce::Colour (0xff0a0a0b));
            g.fillRoundedRectangle (area.translated (0.0f, 3.0f), cornerSize);

            const auto face = area.translated (0.0f, down ? 2.0f : 0.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (down ? 0xff242527 : 0xff2b2c2f), 0.0f, face.getY(),
                                                     juce::Colour (0xff1a1b1d), 0.0f, face.getBottom(), false));
            g.fillRoundedRectangle (face, cornerSize);
            g.setColour (juce::Colours::black);
            g.drawRoundedRectangle (face, cornerSize, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (down ? 0.04f : 0.08f));
            g.drawHorizontalLine ((int) face.getY() + 1, face.getX() + cornerSize, face.getRight() - cornerSize);
            return face;
        }

        void drawLed (juce::Graphics& g, juce::Rectangle<float> area, juce::Colour colour, bool on)
        {
            const auto corner = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;

            if (on)
            {
                g.setColour (colour.withAlpha (0.22f));
                g.fillRoundedRectangle (area.expanded (3.5f), corner + 3.5f);
                g.setColour (colour.withAlpha (0.35f));
                g.fillRoundedRectangle (area.expanded (1.5f), corner + 1.5f);
            }

            g.setColour (on ? colour : colour.withMultipliedBrightness (0.2f).withMultipliedSaturation (0.7f));
            g.fillRoundedRectangle (area, corner);
        }

        void drawKnob (juce::Graphics& g, juce::Point<float> c, float r, float angle,
                       float startAngle, float endAngle, bool bipolar)
        {
            // value ring
            const auto ringRadius = r + 6.0f;
            const auto stroke = juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt);
            juce::Path track;
            track.addCentredArc (c.x, c.y, ringRadius, ringRadius, 0.0f, startAngle, endAngle, true);
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.strokePath (track, stroke);

            const auto from = bipolar ? (startAngle + endAngle) * 0.5f : startAngle;

            if (std::abs (angle - from) > 0.01f)
            {
                juce::Path value;
                value.addCentredArc (c.x, c.y, ringRadius, ringRadius, 0.0f, juce::jmin (from, angle), juce::jmax (from, angle), true);
                g.setColour (ink.withAlpha (0.6f));
                g.strokePath (value, stroke);
            }

            const auto body = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.fillEllipse (body.expanded (1.0f).translated (0.0f, 3.5f));

            juce::ColourGradient skirt (juce::Colour (0xff3a3b3e), c.x, c.y - r * 0.4f, juce::Colour (0xff0c0c0d), c.x, c.y + r * 1.1f, true);
            skirt.addColour (0.45, juce::Colour (0xff1c1d1f));
            g.setGradientFill (skirt);
            g.fillEllipse (body);
            g.setColour (juce::Colours::black);
            g.drawEllipse (body, 1.0f);

            // matte top face with a faint reflection
            const auto top = body.reduced (r * 0.18f);
            const auto tr = top.getWidth() * 0.5f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff313235), top.getCentreX() - tr * 0.2f, top.getCentreY() - tr * 0.4f,
                                                     juce::Colour (0xff151617), top.getCentreX() + tr * 0.5f, top.getBottom(), true));
            g.fillEllipse (top);
            juce::Path rim;
            rim.addCentredArc (c.x, c.y, tr - 0.5f, tr - 0.5f, 0.0f, -1.2f, 1.2f, true);
            g.setColour (juce::Colours::white.withAlpha (0.09f));
            g.strokePath (rim, juce::PathStrokeType (1.0f));

            juce::Path pointer;
            pointer.startNewSubPath (polar (c, angle, r * 0.22f));
            pointer.lineTo (polar (c, angle, r * 0.84f));
            g.setColour (ledWhite);
            g.strokePath (pointer, juce::PathStrokeType (juce::jmax (1.8f, r * 0.09f), juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));
        }

        void drawTrimmer (juce::Graphics& g, juce::Point<float> c, float r, float angle,
                          int numTicks, float startAngle, float endAngle)
        {
            g.setColour (silkscreen.withAlpha (0.7f));

            for (int i = 0; i < numTicks; ++i)
            {
                const auto a = startAngle + (endAngle - startAngle) * (float) i / (float) juce::jmax (1, numTicks - 1);
                g.drawLine (juce::Line<float> (polar (c, a, r + 4.0f), polar (c, a, r + 8.0f)), 1.0f);
            }

            // brushed silver body
            const auto body = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.fillRoundedRectangle (body.translated (1.5f, 2.5f), 3.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe4e6e8), body.getTopLeft(),
                                                     juce::Colour (0xff7d8287), body.getBottomRight(), false));
            g.fillRoundedRectangle (body, 3.0f);
            g.setColour (juce::Colour (0xff3d4045));
            g.drawRoundedRectangle (body, 3.0f, 1.0f);

            // rotor with a cross slot
            const auto rotor = r * 0.72f;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9ccd0), c - juce::Point<float> (rotor, rotor) * 0.4f,
                                                     juce::Colour (0xff5d6268), c + juce::Point<float> (rotor, rotor), true));
            g.fillEllipse (juce::Rectangle<float> (rotor * 2.0f, rotor * 2.0f).withCentre (c));
            g.setColour (juce::Colour (0xff2f3236));
            g.drawEllipse (juce::Rectangle<float> (rotor * 2.0f, rotor * 2.0f).withCentre (c), 0.8f);

            juce::Path slot;
            slot.addRoundedRectangle (-rotor * 0.85f, -1.6f, rotor * 1.7f, 3.2f, 1.0f);
            slot.addRoundedRectangle (-1.6f, -rotor * 0.85f, 3.2f, rotor * 1.7f, 1.0f);
            g.setColour (juce::Colour (0xff1d1f22));
            g.fillPath (slot, juce::AffineTransform::rotation (angle).translated (c));

            g.setColour (ledRed);
            g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (polar (c, angle, rotor * 0.55f)));
        }

        void drawJack (juce::Graphics& g, juce::Point<float> c, const juce::String& label, bool output)
        {
            constexpr float r = 11.0f;
            const auto labelCentre = juce::Point<float> (c.x, c.y - 22.0f);

            if (output)
            {
                // outputs: inverted label, a collar around the socket
                g.setFont (font (9.0f, true));
                const auto w = (float) juce::GlyphArrangement::getStringWidthInt (g.getCurrentFont(), label) + 9.0f;
                const auto tag = juce::Rectangle<float> (w, 13.0f).withCentre (labelCentre);
                g.setColour (ink);
                g.fillRoundedRectangle (tag, 2.0f);
                drawLabel (g, label, labelCentre, 9.0f, juce::Colour (0xff111111));

                g.setColour (juce::Colours::black);
                g.fillEllipse (juce::Rectangle<float> ((r + 5.0f) * 2.0f, (r + 5.0f) * 2.0f).withCentre (c));
                g.setColour (juce::Colour (0xff2c2d30));
                g.fillEllipse (juce::Rectangle<float> ((r + 4.0f) * 2.0f, (r + 4.0f) * 2.0f).withCentre (c));
            }
            else
            {
                drawLabel (g, label, labelCentre, 9.0f, inkDim);
            }

            const auto outer = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillEllipse (outer.translated (0.0f, 1.0f));
            juce::ColourGradient nut (juce::Colour (0xff4b4c50), c.x - r * 0.3f, c.y - r * 0.4f, juce::Colour (0xff1a1b1d), c.x + r, c.y + r, true);
            g.setGradientFill (nut);
            g.fillEllipse (outer);
            g.setColour (juce::Colours::black);
            g.drawEllipse (outer, 1.0f);
            g.setColour (juce::Colour (0xff2c2d30));
            g.fillEllipse (outer.reduced (r * 0.6f));
            g.setColour (juce::Colours::black);
            g.fillEllipse (outer.reduced (r * 0.66f));
        }
    }
}
