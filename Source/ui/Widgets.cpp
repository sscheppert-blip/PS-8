#include "Widgets.h"

#include <PS8Fonts.h>

namespace ps8::ui
{
    namespace
    {
        juce::Point<float> polar (juce::Point<float> centre, float angle, float distance)
        {
            // angle in radians, clockwise from 12 o'clock
            return centre + juce::Point<float> (std::sin (angle), -std::cos (angle)) * distance;
        }
    }

    juce::Font stencilFont (float height)
    {
        static const auto typeface = juce::Typeface::createSystemTypefaceFor (PS8Fonts::SairaStencilOneRegular_ttf,
                                                                              PS8Fonts::SairaStencilOneRegular_ttfSize);
        return juce::Font (juce::FontOptions (typeface).withHeight (height).withKerningFactor (0.04f));
    }

    juce::Font labelFont (float height)
    {
        return juce::Font (juce::FontOptions (height, juce::Font::bold).withHorizontalScale (0.88f).withKerningFactor (0.05f));
    }

    void drawLabel (juce::Graphics& g, const juce::String& text, juce::Point<float> centre, float height,
                    juce::Colour colour, juce::Justification justification)
    {
        g.setFont (labelFont (height));
        g.setColour (colour);
        const auto h = height * 1.4f;
        juce::Rectangle<float> area (centre.x - 100.0f, centre.y - h * 0.5f, 200.0f, h);

        if (justification == juce::Justification::centredRight)
            area.setX (centre.x - 200.0f), area.setWidth (200.0f);
        else if (justification == juce::Justification::centredLeft)
            area.setX (centre.x), area.setWidth (200.0f);

        g.drawText (text, area, justification, false);
    }

    void drawChrome (juce::Graphics& g, juce::Point<float> centre, float radius)
    {
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, centre - juce::Point<float> (radius, radius) * 0.35f,
                                                 juce::Colour (0xff6c706c), centre + juce::Point<float> (radius, radius) * 0.7f, true));
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
        g.setColour (juce::Colour (0xff454845));
        g.drawEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre), 0.8f);
    }

    void drawTrimmer (juce::Graphics& g, juce::Point<float> centre, float r, float angle,
                      int numTicks, float startAngle, float endAngle)
    {
        // silkscreen scale on the board
        g.setColour (colours::silkscreen.withAlpha (0.8f));

        for (int i = 0; i < numTicks; ++i)
        {
            const auto a = startAngle + (endAngle - startAngle) * (float) i / (float) juce::jmax (1, numTicks - 1);
            g.drawLine (juce::Line<float> (polar (centre, a, r + 4.0f), polar (centre, a, r + 8.0f)), 1.0f);
        }

        // square body with a rounded top
        const auto body = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.fillRoundedRectangle (body.translated (1.5f, 2.5f), 3.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4f86d6), body.getTopLeft(),
                                                 juce::Colour (0xff1d4796), body.getBottomRight(), false));
        g.fillRoundedRectangle (body, 3.0f);
        g.setColour (juce::Colour (0xff12305f));
        g.drawRoundedRectangle (body, 3.0f, 1.0f);

        // white rotor with a cross slot
        const auto rotor = r * 0.72f;
        g.setGradientFill (juce::ColourGradient (juce::Colours::white, centre - juce::Point<float> (rotor, rotor) * 0.4f,
                                                 juce::Colour (0xffb8bcc0), centre + juce::Point<float> (rotor, rotor), true));
        g.fillEllipse (juce::Rectangle<float> (rotor * 2.0f, rotor * 2.0f).withCentre (centre));
        g.setColour (juce::Colour (0xff5d6268));
        g.drawEllipse (juce::Rectangle<float> (rotor * 2.0f, rotor * 2.0f).withCentre (centre), 0.8f);

        juce::Path slot;
        slot.addRoundedRectangle (-rotor * 0.85f, -1.6f, rotor * 1.7f, 3.2f, 1.0f);
        slot.addRoundedRectangle (-1.6f, -rotor * 0.85f, 3.2f, rotor * 1.7f, 1.0f);
        g.setColour (juce::Colour (0xff3a3f45));
        g.fillPath (slot, juce::AffineTransform::rotation (angle).translated (centre));

        // pointer notch
        g.setColour (juce::Colour (0xffc8322a));
        g.fillEllipse (juce::Rectangle<float> (3.0f, 3.0f).withCentre (polar (centre, angle, rotor * 0.55f)));
    }

    void drawScrew (juce::Graphics& g, juce::Point<float> centre)
    {
        drawChrome (g, centre, 5.0f);
        g.setColour (juce::Colour (0xff4d524e));
        g.drawLine (centre.x - 3.5f, centre.y - 1.0f, centre.x + 3.5f, centre.y + 1.0f, 1.3f);
    }

    void drawPlate (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title)
    {
        g.setColour (colours::plateShade.withAlpha (0.55f));
        g.fillRoundedRectangle (area.translated (2.0f, 3.0f), 4.0f);
        g.setColour (colours::plate);
        g.fillRoundedRectangle (area, 4.0f);
        g.setColour (colours::frame);
        g.drawRoundedRectangle (area, 4.0f, 1.2f);

        const auto bar = juce::Rectangle<float> (area.getX() + 8.0f, area.getY() + 8.0f, area.getWidth() - 16.0f, 22.0f);
        g.setColour (colours::ink);
        g.fillRect (bar);
        g.setColour (colours::plateText);
        g.setFont (stencilFont (17.0f));
        g.drawText (title, bar, juce::Justification::centred, false);

        drawScrew (g, { area.getX() + 8.0f, area.getBottom() - 8.0f });
        drawScrew (g, { area.getRight() - 8.0f, area.getBottom() - 8.0f });
    }

    void drawHazard (juce::Graphics& g, juce::Rectangle<float> area)
    {
        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (area.toNearestInt());
        g.setColour (colours::hazard);
        g.fillRect (area);
        g.setColour (juce::Colour (0xff1b1b1b));

        const auto h = area.getHeight();

        for (auto x = area.getX() - h; x < area.getRight(); x += 28.0f)
        {
            juce::Path stripe;
            stripe.startNewSubPath (x, area.getBottom());
            stripe.lineTo (x + 14.0f, area.getBottom());
            stripe.lineTo (x + 14.0f + h, area.getY());
            stripe.lineTo (x + h, area.getY());
            stripe.closeSubPath();
            g.fillPath (stripe);
        }
    }

    void drawAppIcon (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const auto size = juce::jmin (area.getWidth(), area.getHeight());
        const auto tile = juce::Rectangle<float> (size, size).withCentre (area.getCentre());
        const auto c = tile.getCentre();
        const auto u = size / 100.0f;   // one percent of the icon

        // panel-green tile
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffa9bfb0), c.x, tile.getY(),
                                                 juce::Colour (0xff5f7368), c.x, tile.getBottom(), false));
        g.fillRoundedRectangle (tile.reduced (2.0f * u), 20.0f * u);
        g.setColour (juce::Colour (0xff3d4a42));
        g.drawRoundedRectangle (tile.reduced (2.5f * u), 19.5f * u, 1.5f * u);

        // hazard ring
        const auto ring = juce::Rectangle<float> (84.0f * u, 84.0f * u).withCentre (c);
        {
            juce::Graphics::ScopedSaveState save (g);
            juce::Path clip;
            clip.addEllipse (ring);
            g.reduceClipRegion (clip);
            g.setColour (colours::hazard);
            g.fillRect (ring);
            g.setColour (juce::Colour (0xff1b1b1b));

            const auto h = ring.getHeight();
            const auto pitch = 16.0f * u;

            for (auto x = ring.getX() - h; x < ring.getRight(); x += pitch)
            {
                juce::Path stripe;
                stripe.startNewSubPath (x, ring.getBottom());
                stripe.lineTo (x + pitch * 0.5f, ring.getBottom());
                stripe.lineTo (x + pitch * 0.5f + h, ring.getY());
                stripe.lineTo (x + h, ring.getY());
                stripe.closeSubPath();
                g.fillPath (stripe);
            }
        }

        g.setColour (juce::Colour (0xff1b1b1b));
        g.drawEllipse (ring, 1.5f * u);

        // nixie tube face with a glowing 8
        const auto face = juce::Rectangle<float> (62.0f * u, 62.0f * u).withCentre (c);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a220d), c.x, c.y - 10.0f * u,
                                                 juce::Colour (0xff120a04), c.x, face.getBottom(), true));
        g.fillEllipse (face);
        g.setColour (juce::Colour (0xff7a6a55));
        g.drawEllipse (face, 1.8f * u);

        const auto font = juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 54.0f * u, juce::Font::bold));
        const auto digitArea = face.translated (0.0f, 1.0f * u);

        constexpr float glow[][2] { { 8.0f, 0.10f }, { 5.0f, 0.18f }, { 2.5f, 0.3f } };   // extra height, alpha

        for (const auto& layer : glow)
        {
            g.setFont (font.withHeight ((54.0f + layer[0]) * u));
            g.setColour (colours::nixie.withAlpha (layer[1]));
            g.drawText ("8", digitArea, juce::Justification::centred, false);
        }

        g.setFont (font);
        g.setColour (juce::Colour (0xffffc070));
        g.drawText ("8", digitArea, juce::Justification::centred, false);

        // glass highlight
        g.setColour (juce::Colours::white.withAlpha (0.10f));
        g.fillEllipse (juce::Rectangle<float> (34.0f * u, 16.0f * u).withCentre (c.translated (-6.0f * u, -19.0f * u)));
    }

    void drawJack (juce::Graphics& g, juce::Point<float> centre, const juce::String& label, bool output)
    {
        drawLabel (g, label, { centre.x, centre.y - 19.0f }, 10.0f);

        if (output)
        {
            g.setColour (colours::ink);
            g.fillRoundedRectangle (juce::Rectangle<float> (28.0f, 28.0f).withCentre (centre), 3.0f);
        }

        drawChrome (g, centre, 10.0f);
        g.setColour (juce::Colour (0xff0b0b0b));
        g.fillEllipse (juce::Rectangle<float> (9.0f, 9.0f).withCentre (centre));
    }

    //==============================================================================
    KnobLookAndFeel::KnobLookAndFeel()
    {
        setColour (juce::TooltipWindow::textColourId, colours::nixie);
    }

    void KnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                            float startAngle, float endAngle, juce::Slider& slider)
    {
        const auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat();
        const auto centre = bounds.getCentre();

        if ((bool) slider.getProperties().getWithDefault ("trimmer", false))
        {
            drawTrimmer (g, centre, juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 14.0f,
                         startAngle + sliderPos * (endAngle - startAngle),
                         (int) slider.getProperties().getWithDefault ("ticks", 11), startAngle, endAngle);
            return;
        }

        const auto r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 14.0f;
        const auto numTicks = (int) slider.getProperties().getWithDefault ("ticks", 11);
        const auto bipolar = (bool) slider.getProperties().getWithDefault ("bipolar", false);

        g.setColour (colours::ink);

        for (int i = 0; i < numTicks; ++i)
        {
            const auto angle = startAngle + (endAngle - startAngle) * (float) i / (float) juce::jmax (1, numTicks - 1);
            const auto major = i == 0 || i == numTicks - 1 || ((bipolar || numTicks == 11) && i * 2 == numTicks - 1);
            g.drawLine (juce::Line<float> (polar (centre, angle, r + 5.0f), polar (centre, angle, r + (major ? 11.0f : 8.0f))),
                        major ? 2.0f : 1.2f);
        }

        const auto knob = juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre);
        g.setColour (juce::Colours::black.withAlpha (0.3f));
        g.fillEllipse (knob.expanded (1.0f).translated (1.5f, 3.0f));

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4a48), centre - juce::Point<float> (r, r) * 0.35f,
                                                 juce::Colour (0xff0d0d0d), centre + juce::Point<float> (r, r) * 0.8f, true));
        g.fillEllipse (knob);
        g.setColour (juce::Colour (0xff080808));
        g.drawEllipse (knob, 1.0f);
        g.setColour (juce::Colour (0xff2e2e2c));
        g.drawEllipse (knob.reduced (r * 0.38f), 1.0f);

        const auto angle = startAngle + sliderPos * (endAngle - startAngle);
        g.setColour (juce::Colour (0xfff2efe6));
        juce::Path pointer;
        pointer.startNewSubPath (polar (centre, angle, r * 0.25f));
        pointer.lineTo (polar (centre, angle, r - 2.0f));
        g.strokePath (pointer, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void KnobLookAndFeel::drawBubble (juce::Graphics& g, juce::BubbleComponent&, const juce::Point<float>&,
                                      const juce::Rectangle<float>& body)
    {
        g.setColour (colours::ink);
        g.fillRoundedRectangle (body, 3.0f);
        g.setColour (colours::nixie.withAlpha (0.6f));
        g.drawRoundedRectangle (body.reduced (0.5f), 3.0f, 1.0f);
    }

    //==============================================================================
    Knob::Knob (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText,
                float knobRadius, int numTicks, bool bipolar, bool withValue)
        : label (labelText), radius (knobRadius), showValue (withValue)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseDragSensitivity (220);
        slider.getProperties().set ("bipolar", bipolar);
        slider.setLookAndFeel (&lookAndFeel.get());
        slider.onValueChange = [this] { if (showValue) repaint(); };
        addAndMakeVisible (slider);

        // only the knob itself takes clicks, so neighbouring labels never cover it
        setInterceptsMouseClicks (false, true);

        bind (state, paramId, numTicks);
    }

    void Knob::bind (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, int numTicks)
    {
        attachment.reset();
        slider.getProperties().set ("ticks", numTicks);
        parameter = state.getParameter (paramId);
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (state, paramId, slider);

        if (parameter != nullptr)
            slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));

        repaint();
    }

    void Knob::setTrimmerStyle()
    {
        trimmer = true;
        slider.getProperties().set ("trimmer", true);
        repaint();
    }

    Knob::~Knob()
    {
        slider.setLookAndFeel (nullptr);
    }

    void Knob::parentHierarchyChanged()
    {
        // The value bubble lives inside the plugin window: as a separate desktop window it could
        // swallow the second click of a double-click in some hosts.
        if (auto* top = getTopLevelComponent(); top != nullptr && top != this)
            slider.setPopupDisplayEnabled (true, true, top);
    }

    void Knob::place (juce::Point<float> centre)
    {
        const auto box = radius + 14.0f;
        const auto width = juce::jmax (box * 2.0f, 110.0f);
        setBounds (juce::Rectangle<float> (centre.x - width * 0.5f, centre.y - box, width,
                                           box * 2.0f + (showValue ? 38.0f : 20.0f)).toNearestInt());
    }

    void Knob::resized()
    {
        const auto box = juce::roundToInt ((radius + 14.0f) * 2.0f);
        slider.setBounds ((getWidth() - box) / 2, 0, box, box);
    }

    void Knob::paint (juce::Graphics& g)
    {
        const auto bottom = (radius + 14.0f) * 2.0f;

        if (label.isNotEmpty())
            drawLabel (g, label, { (float) getWidth() * 0.5f, bottom + 6.0f }, trimmer ? 11.0f : 12.0f,
                       trimmer ? colours::silkscreen : colours::ink);

        if (showValue && parameter != nullptr)
        {
            const auto tag = juce::Rectangle<float> (70.0f, 15.0f).withCentre ({ (float) getWidth() * 0.5f, bottom + 24.0f });
            g.setColour (trimmer ? juce::Colour (0xff0d2618) : colours::ink);
            g.fillRoundedRectangle (tag, 2.0f);

            if (trimmer)
            {
                g.setColour (colours::silkscreen.withAlpha (0.5f));
                g.drawRoundedRectangle (tag, 2.0f, 0.8f);
            }

            drawLabel (g, parameter->getCurrentValueAsText().toUpperCase(), tag.getCentre(), 10.0f,
                       trimmer ? colours::led : colours::nixie);
        }
    }

    //==============================================================================
    Stepper::Stepper (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, const juce::String& labelText,
                      float labelSpace)
        : parameter (*state.getParameter (paramId)),
          label (labelText),
          labelWidth (labelSpace),
          attachment (parameter, [this] (float v)
          {
              value = v;
              repaint();
          })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    juce::Rectangle<float> Stepper::box() const
    {
        return getLocalBounds().toFloat().withTrimmedLeft (labelWidth).reduced (0.0f, 2.0f);
    }

    void Stepper::stepBy (int delta)
    {
        const auto& range = parameter.getNormalisableRange();
        const auto next = juce::jlimit (range.start, range.end, std::round (value) + (float) delta);

        if (std::abs (next - value) > 0.5f)
            attachment.setValueAsCompleteGesture (next);
    }

    void Stepper::showList()
    {
        const auto& range = parameter.getNormalisableRange();
        juce::PopupMenu menu;

        for (auto v = range.start; v <= range.end + 0.5f; v += 1.0f)
            menu.addItem (juce::roundToInt (v - range.start) + 1, parameter.getText (parameter.convertTo0to1 (v), 32),
                          true, std::abs (v - value) < 0.5f);

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [this, start = range.start] (int result)
                            {
                                if (result > 0)
                                    attachment.setValueAsCompleteGesture (start + (float) (result - 1));
                            });
    }

    void Stepper::mouseUp (const juce::MouseEvent& e)
    {
        const auto b = box();

        if (! b.contains (e.position) || e.mouseWasDraggedSinceMouseDown())
            return;

        if (e.position.x < b.getX() + 16.0f)
            stepBy (-1);
        else if (e.position.x > b.getRight() - 16.0f)
            stepBy (1);
        else
            showList();
    }

    void Stepper::mouseDoubleClick (const juce::MouseEvent& e)
    {
        const auto b = box();

        // only the value itself resets; double-clicking an arrow just steps twice
        if (b.contains (e.position) && e.position.x >= b.getX() + 16.0f && e.position.x <= b.getRight() - 16.0f)
            attachment.setValueAsCompleteGesture (parameter.convertFrom0to1 (parameter.getDefaultValue()));
    }

    void Stepper::mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails& wheelDetails)
    {
        wheel += wheelDetails.deltaY * (wheelDetails.isReversed ? -1.0f : 1.0f);

        if (std::abs (wheel) >= 0.12f)
        {
            stepBy (wheel > 0.0f ? 1 : -1);
            wheel = 0.0f;
        }
    }

    void Stepper::paint (juce::Graphics& g)
    {
        drawLabel (g, label, { 2.0f, (float) getHeight() * 0.5f }, 10.0f, colours::dimText, juce::Justification::centredLeft);

        const auto b = box();
        g.setColour (juce::Colour (0xff0d120f));
        g.fillRoundedRectangle (b, 2.0f);
        g.setColour (colours::plateText.withAlpha (0.6f));
        g.drawRoundedRectangle (b, 2.0f, 1.0f);

        const auto& range = parameter.getNormalisableRange();
        auto arrow = [&g] (juce::Point<float> c, bool left, bool enabled)
        {
            juce::Path p;
            const auto d = left ? -1.0f : 1.0f;
            p.addTriangle (c.x + 3.5f * d, c.y, c.x - 3.0f * d, c.y - 4.0f, c.x - 3.0f * d, c.y + 4.0f);
            g.setColour (enabled ? colours::plateText : colours::plateText.withAlpha (0.25f));
            g.fillPath (p);
        };

        arrow ({ b.getX() + 9.0f, b.getCentreY() }, true, value > range.start + 0.5f);
        arrow ({ b.getRight() - 9.0f, b.getCentreY() }, false, value < range.end - 0.5f);

        g.setColour (colours::nixie);
        g.setFont (labelFont (12.0f));
        g.drawText (parameter.getCurrentValueAsText(), b.reduced (16.0f, 0.0f), juce::Justification::centred, false);
    }

    //==============================================================================
    SelectorKeys::SelectorKeys (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
                                juce::StringArray keyNames)
        : names (std::move (keyNames)),
          attachment (*state.getParameter (paramId), [this] (float value)
          {
              selected = juce::jlimit (0, names.size() - 1, juce::roundToInt (value));
              repaint();
          })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    juce::Rectangle<float> SelectorKeys::keyBounds (int index) const
    {
        const auto h = (float) getHeight() / (float) names.size();
        return { 1.0f, h * (float) index + 1.0f, (float) getWidth() - 2.0f, h - 2.0f };
    }

    void SelectorKeys::mouseUp (const juce::MouseEvent& e)
    {
        for (int i = 0; i < names.size(); ++i)
            if (keyBounds (i).contains (e.position))
                attachment.setValueAsCompleteGesture ((float) i);
    }

    void SelectorKeys::paint (juce::Graphics& g)
    {
        for (int i = 0; i < names.size(); ++i)
        {
            const auto key = keyBounds (i);
            const auto lit = i == selected;
            g.setColour (lit ? colours::plateText : colours::ink.brighter (0.25f));
            g.fillRoundedRectangle (key, 2.0f);
            g.setColour (colours::plateText.withAlpha (0.6f));
            g.drawRoundedRectangle (key, 2.0f, 1.0f);
            drawLabel (g, names[i], key.getCentre().translated (5.0f, 0.0f), 9.5f, lit ? colours::ink : colours::plateText);

            g.setColour (lit ? colours::amber : juce::Colour (0xff3b3020));
            g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ key.getX() + 8.0f, key.getCentreY() }));
        }
    }

    //==============================================================================
    Toggle::Toggle (juce::AudioProcessorValueTreeState& state, const juce::String& paramId, std::vector<float> leverPositions,
                    Labels toggleLabels)
        : positions (std::move (leverPositions)),
          labels (std::move (toggleLabels)),
          attachment (*state.getParameter (paramId), [this] (float value)
          {
              int nearest = 0;

              for (size_t i = 1; i < positions.size(); ++i)
                  if (std::abs (positions[i] - value) < std::abs (positions[(size_t) nearest] - value))
                      nearest = (int) i;

              index = nearest;
              repaint();
          })
    {
        attachment.sendInitialUpdate();
    }

    juce::Point<float> Toggle::centre() const { return { 38.0f, 84.0f }; }

    void Toggle::place (juce::Point<float> c)
    {
        setBounds (juce::roundToInt (c.x - 38.0f), juce::roundToInt (c.y - 84.0f), 76, 152);
    }

    void Toggle::setIndex (int newIndex)
    {
        index = newIndex;
        attachment.setValueAsCompleteGesture (positions[(size_t) index]);
        repaint();
    }

    void Toggle::mouseUp (const juce::MouseEvent& e)
    {
        const auto c = centre();
        const auto last = (int) positions.size() - 1;

        if (last == 1)
            setIndex (1 - index);
        else
            setIndex (e.position.y < c.y ? juce::jmax (0, index - 1) : juce::jmin (last, index + 1));
    }

    void Toggle::paint (juce::Graphics& g)
    {
        const auto c = centre();

        drawLabel (g, labels.top, { c.x, c.y - 31.0f }, 10.0f);
        drawLabel (g, labels.bottom, { c.x, c.y + 40.0f }, 10.0f);

        if (labels.middle.isNotEmpty())
            drawLabel (g, labels.middle, { c.x + 17.0f, c.y }, 10.0f, colours::ink, juce::Justification::centredLeft);

        if (labels.name.isNotEmpty())
            drawLabel (g, labels.name, { c.x, labels.nameAbove ? c.y - 46.0f : c.y + 58.0f }, 12.0f);

        g.setColour (juce::Colour (0xff2a2f2b));
        g.fillRoundedRectangle (c.x - 13.0f, c.y - 21.0f, 26.0f, 42.0f, 3.0f);
        drawChrome (g, c, 9.0f);

        const auto last = (int) positions.size() - 1;
        const auto dy = index == 0 ? -22.0f : (index == last ? 22.0f : -5.0f);
        const auto tip = c.translated (0.0f, dy);

        g.setColour (juce::Colour (0xffd8dbd6));
        g.drawLine (juce::Line<float> (c, tip), 5.0f);
        drawChrome (g, tip, index == 0 || index == last ? 4.2f : 5.5f);
    }

    //==============================================================================
    LampButton::LampButton (juce::AudioProcessorValueTreeState& state, const juce::String& paramId)
        : attachment (*state.getParameter (paramId), [this] (float value)
          {
              on = value > 0.5f;
              repaint();
          })
    {
        attachment.sendInitialUpdate();
    }

    void LampButton::place (juce::Point<float> c)
    {
        setBounds (juce::roundToInt (c.x - 20.0f), juce::roundToInt (c.y - 20.0f), 40, 40);
    }

    void LampButton::setState (bool isCurrent, bool isAlarm, bool isInRange)
    {
        if (current != isCurrent || alarm != isAlarm || inRange != isInRange)
        {
            current = isCurrent;
            alarm = isAlarm;
            inRange = isInRange;
            repaint();
        }
    }

    void LampButton::mouseUp (const juce::MouseEvent& e)
    {
        if (getLocalBounds().contains (e.getPosition()))
            attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
    }

    void LampButton::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (colours::ink);
        g.fillRoundedRectangle (bounds, 3.0f);

        auto face = juce::Colour (on ? (inRange ? 0xff6b8a5a : 0xff3c4d3a) : 0xff2c362f);

        if (current)
            face = on ? colours::amber : juce::Colour (0xff7a5a2a);

        if (alarm)
            face = juce::Colour (current ? 0xffe0301e : 0xff3b1210);

        const auto inner = bounds.reduced (4.0f);
        g.setColour (face);
        g.fillRoundedRectangle (inner, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (current ? 0.35f : 0.12f));
        g.fillRoundedRectangle (inner.reduced (5.0f).withHeight (6.0f), 2.0f);
        g.setColour (juce::Colour (0xff0f1611));
        g.drawRoundedRectangle (inner, 2.0f, 1.0f);
    }

    //==============================================================================
    PushButton::PushButton (const juce::String& labelText, bool isScram)
        : label (labelText), scram (isScram)
    {
    }

    void PushButton::place (juce::Point<float> c, float r)
    {
        radius = r;

        if (scram)
            setBounds (juce::Rectangle<float> (108.0f, 108.0f).withCentre (c).toNearestInt());
        else
            setBounds (juce::roundToInt (c.x - 24.0f), juce::roundToInt (c.y - r - 5.0f), 48, juce::roundToInt (2.0f * r + 30.0f));
    }

    void PushButton::mouseDown (const juce::MouseEvent&)
    {
        down = true;
        repaint();

        if (onPress)
            onPress();
    }

    void PushButton::mouseUp (const juce::MouseEvent&)
    {
        down = false;
        repaint();
    }

    void PushButton::paint (juce::Graphics& g)
    {
        if (scram)
        {
            const auto c = getLocalBounds().toFloat().getCentre();

            {
                juce::Graphics::ScopedSaveState save (g);
                juce::Path ring;
                ring.addEllipse (juce::Rectangle<float> (100.0f, 100.0f).withCentre (c));
                g.reduceClipRegion (ring);
                drawHazard (g, juce::Rectangle<float> (100.0f, 100.0f).withCentre (c));
            }

            g.setColour (juce::Colour (0xff1b1b1b));
            g.fillEllipse (juce::Rectangle<float> (76.0f, 76.0f).withCentre (c));
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (64.0f, 64.0f).withCentre (c.translated (2.0f, 4.0f)));

            const auto bc = down ? c.translated (1.0f, 2.0f) : c;
            const auto r = down ? radius * 0.96f : radius;
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff6a55), bc - juce::Point<float> (r, r) * 0.35f,
                                                     juce::Colour (0xff6e0f0a), bc + juce::Point<float> (r, r) * 0.8f, true));
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (bc));
            g.setColour (juce::Colour (0xff4a0905));
            g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (bc), 1.5f);
            g.setColour (juce::Colour (0xffffe9e4));
            g.setFont (stencilFont (20.0f));
            g.drawText (label, juce::Rectangle<float> (80.0f, 20.0f).withCentre (bc), juce::Justification::centred, false);
            return;
        }

        const auto c = juce::Point<float> ((float) getWidth() * 0.5f, radius + 5.0f);
        drawChrome (g, c, radius + 4.0f);
        g.setColour (down ? juce::Colour (0xffb08a18) : colours::hazard);
        g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (c).reduced (down ? 1.0f : 0.0f));
        g.setColour (juce::Colours::white.withAlpha (down ? 0.1f : 0.3f));
        g.fillEllipse (juce::Rectangle<float> (radius, radius * 0.6f).withCentre (c.translated (-radius * 0.2f, -radius * 0.4f)));
        drawLabel (g, label, { c.x, c.y + radius + 14.0f }, 10.0f);
    }

    //==============================================================================
    Nixie::Nixie (int digits) : numDigits (digits)
    {
        setSize (juce::roundToInt (tubePitch * (float) numDigits - (tubePitch - tubeWidth)), (int) tubeHeight);
    }

    void Nixie::setText (const juce::String& newText)
    {
        if (text != newText)
        {
            text = newText;
            repaint();
        }
    }

    void Nixie::paint (juce::Graphics& g)
    {
        const auto padded = juce::String::repeatedString (" ", juce::jmax (0, numDigits - text.length())) + text;
        const auto font = juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 32.0f, juce::Font::plain));

        for (int i = 0; i < numDigits; ++i)
        {
            const auto tube = juce::Rectangle<float> (tubePitch * (float) i, 0.0f, tubeWidth, tubeHeight);
            g.setColour (juce::Colour (0xff2a1a0d));
            g.fillRoundedRectangle (tube, 13.0f);
            g.setColour (juce::Colour (0xff7a6a55));
            g.drawRoundedRectangle (tube.reduced (0.75f), 13.0f, 1.5f);
            g.setColour (juce::Colour (0xff3a220d).withAlpha (0.7f));
            g.fillRoundedRectangle (tube.reduced (6.0f), 6.0f);

            const auto digit = padded.substring (i, i + 1);

            if (digit.isNotEmpty() && digit != " ")
            {
                g.setFont (font.withHeight (36.0f));
                g.setColour (colours::nixie.withAlpha (0.25f));
                g.drawText (digit, tube.translated (0.0f, 1.0f), juce::Justification::centred, false);
                g.setFont (font);
                g.setColour (colours::nixie);
                g.drawText (digit, tube.translated (0.0f, 1.0f), juce::Justification::centred, false);
            }
        }
    }

    //==============================================================================
    Gauge::Gauge (const juce::String& labelText, juce::StringArray scaleMarks)
        : label (labelText), marks (std::move (scaleMarks))
    {
    }

    void Gauge::setValue (float newValue)
    {
        newValue = juce::jlimit (0.0f, 1.0f, newValue);

        if (std::abs (newValue - value) > 0.002f)
        {
            value = newValue;
            repaint();
        }
    }

    void Gauge::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (colours::ink);
        g.fillRoundedRectangle (bounds, 6.0f);

        const auto face = bounds.reduced (4.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4ecd2), face.getCentreX(), face.getY(),
                                                 juce::Colour (0xffd8cdaa), face.getCentreX(), face.getBottom(), false));
        g.fillRoundedRectangle (face, 3.0f);

        const auto pivot = juce::Point<float> (face.getCentreX(), face.getBottom() + 18.0f);
        const auto r = face.getHeight() - 2.0f;
        const auto degrees = [] (float d) { return juce::degreesToRadians (d); };

        juce::Graphics::ScopedSaveState save (g);
        g.reduceClipRegion (face.toNearestInt());

        for (int i = 0; i <= 10; ++i)
        {
            const auto angle = degrees (-50.0f + 10.0f * (float) i);
            const auto major = i % 5 == 0;
            g.setColour (i > 7 ? juce::Colour (0xffb3261e) : juce::Colour (0xff2a2a28));
            g.drawLine (juce::Line<float> (polar (pivot, angle, r - 12.0f), polar (pivot, angle, r - (major ? 22.0f : 18.0f))),
                        major ? 1.8f : 1.0f);
        }

        for (int i = 0; i < marks.size(); ++i)
        {
            const auto angle = degrees (-50.0f + 100.0f * (float) i / (float) juce::jmax (1, marks.size() - 1));
            drawLabel (g, marks[i], polar (pivot, angle, r - 32.0f).translated (0.0f, 2.0f), 10.0f);
        }

        drawLabel (g, label, { face.getCentreX(), face.getBottom() - 9.0f }, 11.0f);

        g.setColour (juce::Colour (0xffb3261e));
        g.drawLine (juce::Line<float> (pivot, polar (pivot, degrees (-50.0f + 100.0f * value), r - 8.0f)), 2.0f);

        g.setColour (colours::ink);
        g.fillRect (face.withTop (face.getBottom() - 2.0f));
    }

    //==============================================================================
    //==============================================================================
    Lamp::Lamp (juce::Colour onCol, juce::Colour offCol) : onColour (onCol), offColour (offCol)
    {
        setInterceptsMouseClicks (false, false);
    }

    void Lamp::setOn (bool shouldBeOn)
    {
        if (on != shouldBeOn)
        {
            on = shouldBeOn;
            repaint();
        }
    }

    void Lamp::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat().reduced (2.0f);

        if (on)
        {
            g.setColour (onColour.withAlpha (0.3f));
            g.fillEllipse (getLocalBounds().toFloat());
        }

        g.setColour (on ? onColour : offColour);
        g.fillEllipse (bounds.reduced (2.0f));
        g.setColour (juce::Colour (0xff0c1a0e));
        g.drawEllipse (bounds.reduced (2.0f), 2.0f);

        if (on)
        {
            g.setColour (juce::Colours::white.withAlpha (0.4f));
            g.fillEllipse (bounds.reduced (bounds.getWidth() * 0.3f).translated (-1.5f, -1.5f));
        }
    }
}
