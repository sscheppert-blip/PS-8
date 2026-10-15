#include "ServiceBoard.h"
#include "Parameters.h"

namespace ps8::ui
{
    namespace
    {
        namespace id = params::id;

        constexpr float holeOffsetX = 7.0f, holeOffsetY = 5.0f;

        // trimmer row on the board
        constexpr float trimmerY = 98.0f, trimmerRadius = 13.0f;

        void drawChip (juce::Graphics& g, juce::Rectangle<float> body, int pinsPerSide, const juce::String& name)
        {
            // pins
            g.setColour (juce::Colour (0xffb9bec4));
            const auto pitch = body.getWidth() / (float) pinsPerSide;

            for (int i = 0; i < pinsPerSide; ++i)
            {
                const auto x = body.getX() + pitch * ((float) i + 0.5f) - 2.0f;
                g.fillRect (x, body.getY() - 4.0f, 4.0f, 4.0f);
                g.fillRect (x, body.getBottom(), 4.0f, 4.0f);
            }

            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRoundedRectangle (body.translated (1.5f, 2.0f), 2.0f);
            g.setColour (juce::Colour (0xff1b1c1e));
            g.fillRoundedRectangle (body, 2.0f);
            g.setColour (juce::Colour (0xff35373b));
            g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ body.getX() + 6.0f, body.getCentreY() }));
            drawLabel (g, name, body.getCentre().translated (4.0f, 0.0f), 9.0f, juce::Colour (0xffc9ccd0));
        }

        void drawCapacitor (juce::Graphics& g, juce::Point<float> centre, float r)
        {
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre.translated (1.5f, 2.0f)));
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3c4f7a), centre - juce::Point<float> (r, r) * 0.5f,
                                                     juce::Colour (0xff17213a), centre + juce::Point<float> (r, r), true));
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (centre));
            g.setColour (juce::Colour (0xffaab4c8));
            g.drawEllipse (juce::Rectangle<float> (r * 1.3f, r * 1.3f).withCentre (centre), 1.0f);

            // minus stripe
            juce::Path stripe;
            stripe.addCentredArc (centre.x, centre.y, r - 1.5f, r - 1.5f, 0.0f, 0.6f, 1.6f, true);
            g.setColour (juce::Colour (0xffd5dbe6).withAlpha (0.8f));
            g.strokePath (stripe, juce::PathStrokeType (2.5f));
        }

        void drawResistor (juce::Graphics& g, juce::Point<float> centre, std::initializer_list<juce::uint32> bands)
        {
            g.setColour (juce::Colour (0xffb9bec4));
            g.drawLine (centre.x - 18.0f, centre.y, centre.x + 18.0f, centre.y, 1.2f);
            const auto body = juce::Rectangle<float> (22.0f, 8.0f).withCentre (centre);
            g.setColour (juce::Colour (0xffd8c39a));
            g.fillRoundedRectangle (body, 3.0f);

            auto x = body.getX() + 4.0f;

            for (auto band : bands)
            {
                g.setColour (juce::Colour (band));
                g.fillRect (x, body.getY(), 2.5f, body.getHeight());
                x += 4.5f;
            }
        }

        void drawHole (juce::Graphics& g, juce::Point<float> centre)
        {
            g.setColour (juce::Colour (0xffc9a24a));
            g.fillEllipse (juce::Rectangle<float> (14.0f, 14.0f).withCentre (centre));
            g.setColour (juce::Colour (0xff101410));
            g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre (centre));
        }

        void drawTrace (juce::Graphics& g, std::initializer_list<juce::Point<float>> points)
        {
            juce::Path path;
            bool first = true;

            for (auto p : points)
            {
                if (first)
                    path.startNewSubPath (p);
                else
                    path.lineTo (p);

                first = false;
            }

            g.setColour (colours::copper);
            g.strokePath (path, juce::PathStrokeType (2.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }

        void drawSection (juce::Graphics& g, juce::Rectangle<float> area, const juce::String& title)
        {
            g.setColour (colours::silkscreen.withAlpha (0.7f));
            g.drawRoundedRectangle (area, 4.0f, 1.0f);
            const auto titleArea = juce::Rectangle<float> (area.getX() + 10.0f, area.getY() - 7.0f, 140.0f, 14.0f);
            g.setColour (colours::pcb);
            g.fillRect (titleArea.withWidth ((float) title.length() * 7.5f + 12.0f));
            drawLabel (g, title, { titleArea.getX() + 6.0f, titleArea.getCentreY() }, 10.0f, colours::silkscreen,
                       juce::Justification::centredLeft);
        }
    }

    //==============================================================================
    Jumper::Jumper (juce::AudioProcessorValueTreeState& state, const juce::String& paramId,
                    juce::String top, juce::String bottom, juce::String nameText)
        : topLabel (std::move (top)), bottomLabel (std::move (bottom)), name (std::move (nameText)),
          attachment (*state.getParameter (paramId), [this] (float value)
          {
              on = value > 0.5f;
              repaint();
          })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void Jumper::place (juce::Point<float> centre)
    {
        setBounds (juce::roundToInt (centre.x - 45.0f), juce::roundToInt (centre.y - 40.0f), 90, 80);
    }

    void Jumper::mouseUp (const juce::MouseEvent& e)
    {
        if (getLocalBounds().contains (e.getPosition()))
            attachment.setValueAsCompleteGesture (on ? 0.0f : 1.0f);
    }

    void Jumper::paint (juce::Graphics& g)
    {
        const auto c = juce::Point<float> (30.0f, 44.0f);
        drawLabel (g, name, { 45.0f, 10.0f }, 10.0f, colours::silkscreen);

        // header outline and pins
        g.setColour (colours::silkscreen.withAlpha (0.7f));
        g.drawRect (juce::Rectangle<float> (16.0f, 44.0f).withCentre (c), 1.0f);

        for (int i = -1; i <= 1; ++i)
        {
            const auto pin = c.translated (0.0f, 14.0f * (float) i);
            g.setColour (juce::Colour (0xffd6b25a));
            g.fillRect (juce::Rectangle<float> (7.0f, 7.0f).withCentre (pin));
            g.setColour (juce::Colour (0xff6d5521));
            g.fillRect (juce::Rectangle<float> (3.0f, 3.0f).withCentre (pin));
        }

        // the cap bridges two pins
        const auto capCentre = c.translated (0.0f, on ? 7.0f : -7.0f);
        const auto cap = juce::Rectangle<float> (13.0f, 26.0f).withCentre (capCentre);
        g.setColour (juce::Colours::black.withAlpha (0.4f));
        g.fillRoundedRectangle (cap.translated (1.5f, 2.0f), 2.0f);
        g.setColour (juce::Colour (0xff17181a));
        g.fillRoundedRectangle (cap, 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.fillRoundedRectangle (cap.reduced (3.0f, 4.0f).withWidth (3.0f), 1.0f);

        drawLabel (g, topLabel, { 44.0f, c.y - 7.0f }, 10.0f, on ? colours::silkscreen.withAlpha (0.5f) : colours::silkscreen,
                   juce::Justification::centredLeft);
        drawLabel (g, bottomLabel, { 44.0f, c.y + 7.0f }, 10.0f, on ? colours::silkscreen : colours::silkscreen.withAlpha (0.5f),
                   juce::Justification::centredLeft);
    }

    //==============================================================================
    Led::Led (juce::String text, juce::Colour c) : caption (std::move (text)), colour (c)
    {
        setInterceptsMouseClicks (false, false);
    }

    void Led::setOn (bool shouldBeOn)
    {
        if (on != shouldBeOn)
        {
            on = shouldBeOn;
            repaint();
        }
    }

    void Led::paint (juce::Graphics& g)
    {
        const auto c = juce::Point<float> (10.0f, (float) getHeight() * 0.5f);

        if (on)
        {
            g.setColour (colour.withAlpha (0.35f));
            g.fillEllipse (juce::Rectangle<float> (18.0f, 18.0f).withCentre (c));
        }

        g.setColour (on ? colour : colour.darker (2.2f));
        g.fillEllipse (juce::Rectangle<float> (10.0f, 10.0f).withCentre (c));
        g.setColour (juce::Colours::white.withAlpha (on ? 0.6f : 0.15f));
        g.fillEllipse (juce::Rectangle<float> (3.5f, 3.5f).withCentre (c.translated (-1.5f, -1.5f)));
        drawLabel (g, caption, { 24.0f, c.y }, 10.0f, colours::silkscreen, juce::Justification::centredLeft);
    }

    //==============================================================================
    KeySwitch::KeySwitch (juce::AudioProcessorValueTreeState& state, const juce::String& paramId)
        : attachment (*state.getParameter (paramId), [this] (float value)
          {
              open = value > 0.5f;
              repaint();
          })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void KeySwitch::place (juce::Point<float> centre)
    {
        setBounds (juce::roundToInt (centre.x - 30.0f), juce::roundToInt (centre.y - 22.0f), 60, 62);
    }

    void KeySwitch::mouseUp (const juce::MouseEvent& e)
    {
        if (getLocalBounds().contains (e.getPosition()))
            attachment.setValueAsCompleteGesture (open ? 0.0f : 1.0f);
    }

    void KeySwitch::paint (juce::Graphics& g)
    {
        const auto c = juce::Point<float> (30.0f, 22.0f);
        drawChrome (g, c, 17.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3c3f), c.translated (-5.0f, -6.0f),
                                                 juce::Colour (0xff0e0f10), c.translated (8.0f, 10.0f), true));
        g.fillEllipse (juce::Rectangle<float> (26.0f, 26.0f).withCentre (c));

        // key in the lock, turned a quarter when open
        const auto angle = open ? juce::MathConstants<float>::halfPi : 0.0f;
        juce::Path key;
        key.addRoundedRectangle (-2.5f, -10.0f, 5.0f, 20.0f, 1.5f);
        g.setColour (juce::Colour (0xffcaa64a));
        g.fillPath (key, juce::AffineTransform::rotation (angle).translated (c));
        g.setColour (juce::Colour (0xff6d5521));
        g.strokePath (key, juce::PathStrokeType (0.8f), juce::AffineTransform::rotation (angle).translated (c));

        // position marks
        g.setColour (open ? colours::amber : colours::dimText);
        g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (c.translated (21.0f, -6.0f)));
        drawLabel (g, "SERVICE", { 30.0f, 52.0f }, 10.0f, open ? colours::amber : colours::ink);
    }

    //==============================================================================
    PageTabs::PageTabs (juce::AudioProcessorValueTreeState& state, const juce::String& paramId)
        : attachment (*state.getParameter (paramId), [this] (float value)
          {
              shown = value > 0.5f ? 1 : 0;
              repaint();
          })
    {
        attachment.sendInitialUpdate();
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    void PageTabs::setPlayingPage (int page)
    {
        if (playing != page)
        {
            playing = page;
            repaint();
        }
    }

    void PageTabs::mouseUp (const juce::MouseEvent& e)
    {
        if (getLocalBounds().contains (e.getPosition()))
            attachment.setValueAsCompleteGesture (e.position.x < (float) getWidth() * 0.5f ? 0.0f : 1.0f);
    }

    void PageTabs::paint (juce::Graphics& g)
    {
        const char* names[] { "1-8", "9-16" };
        const auto w = ((float) getWidth() - 6.0f) * 0.5f;

        for (int page = 0; page < 2; ++page)
        {
            const auto tab = juce::Rectangle<float> (2.0f + (w + 2.0f) * (float) page, 2.0f, w, (float) getHeight() - 4.0f);
            const auto selected = page == shown;
            g.setColour (selected ? colours::plateText : colours::ink.brighter (0.25f));
            g.fillRoundedRectangle (tab, 2.0f);
            g.setColour (colours::plateText.withAlpha (0.6f));
            g.drawRoundedRectangle (tab, 2.0f, 1.0f);
            drawLabel (g, names[page], tab.getCentre().translated (4.0f, 0.0f), 10.0f, selected ? colours::ink : colours::plateText);

            // this page is playing
            g.setColour (page == playing ? colours::amber : juce::Colour (0xff3b3020));
            g.fillEllipse (juce::Rectangle<float> (6.0f, 6.0f).withCentre ({ tab.getX() + 8.0f, tab.getCentreY() }));
        }
    }

    //==============================================================================
    Hood::Hood()
    {
        setInterceptsMouseClicks (true, true);
    }

    std::array<juce::Point<float>, 4> Hood::screwPositions (juce::Rectangle<float> area)
    {
        return { { { area.getX() + holeOffsetX, area.getY() + holeOffsetY },
                   { area.getRight() - holeOffsetX, area.getY() + holeOffsetY },
                   { area.getX() + holeOffsetX, area.getBottom() - holeOffsetY },
                   { area.getRight() - holeOffsetX, area.getBottom() - holeOffsetY } } };
    }

    void Hood::setScrews (float progress)
    {
        if (std::abs (progress - screws) > 1.0e-4f)
        {
            screws = progress;
            repaint();
        }
    }

    void Hood::setShade (float amount)
    {
        if (std::abs (amount - shade) > 1.0e-4f)
        {
            shade = amount;
            repaint();
        }
    }

    bool Hood::hitTest (int, int)
    {
        return true;   // the cover hides the board completely
    }

    void Hood::mouseUp (const juce::MouseEvent& e)
    {
        for (auto p : screwPositions (getLocalBounds().toFloat()))
            if (p.getDistanceFrom (e.position) <= 9.0f && onUnscrew)
                onUnscrew();
    }

    void Hood::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        g.setColour (colours::panel);
        g.fillRect (bounds);

        // the cover's edge: a fine groove around it
        g.setColour (colours::frame.darker (0.4f).withAlpha (0.7f));
        g.drawRect (bounds.reduced (0.5f), 1.0f);

        if (painter)
        {
            juce::Graphics::ScopedSaveState save (g);
            g.addTransform (juce::AffineTransform::translation ((float) -getX(), (float) -getY()));
            painter (g);
        }

        for (auto p : screwPositions (bounds))
        {
            g.setColour (juce::Colour (0xff101410));
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (p));

            if (screws >= 1.0f)
                continue;

            // the screw turns out and rises towards the viewer
            const auto lift = screws;
            const auto head = p.translated (-1.5f * lift, -3.0f * lift);
            const auto r = 5.5f + 1.5f * lift;
            g.setColour (juce::Colours::black.withAlpha (0.25f + 0.25f * lift));
            g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (p.translated (1.0f + 2.0f * lift, 2.0f + 2.0f * lift)));
            drawChrome (g, head, r);

            const auto angle = -lift * juce::MathConstants<float>::twoPi * 3.0f + 0.3f;
            const auto dir = juce::Point<float> (std::cos (angle), std::sin (angle)) * (r - 1.5f);
            g.setColour (juce::Colour (0xff4d524e));
            g.drawLine (juce::Line<float> (head - dir, head + dir), 1.4f);
        }

        if (shade > 0.0f)
        {
            g.setColour (juce::Colours::black.withAlpha (0.65f * shade));
            g.fillRect (bounds);
        }
    }

    //==============================================================================
    ServiceBoard::ServiceBoard (PS8AudioProcessor& p) : processor (p)
    {
        // sequencer options (RATE and LENGTH are on the front, in the sequencer's title bar)
        trimmer (id::scale, "SCALE", 76.0f, 5);
        trimmer (id::root, "ROOT", 172.0f, 12);
        trimmer (id::glide, "GLIDE", 268.0f, 11);
        trimmer (id::swing, "SWING", 364.0f, 11);

        // LFO
        lfoRate = &trimmer (id::lfoRate, "RATE", 500.0f, 11);
        trimmer (id::lfoShape, "SHAPE", 596.0f, 5);
        trimmer (id::lfoDepth, "DEPTH", 692.0f, 11);
        trimmer (id::lfoTarget, "TARGET", 788.0f, 5);

        auto& sync = *parts.emplace_back (std::make_unique<Jumper> (processor.getState(), id::lfoSync, "FREE", "SYNC", "LFO SYNC"));
        static_cast<Jumper&> (sync).place ({ 880.0f, trimmerY + 4.0f });
        addAndMakeVisible (sync);

        // status LEDs
        auto led = [this] (const char* caption, juce::Colour colour, float y)
        {
            auto& l = static_cast<Led&> (*parts.emplace_back (std::make_unique<Led> (caption, colour)));
            l.setBounds (968, juce::roundToInt (y) - 9, 170, 18);
            addAndMakeVisible (l);
            return &l;
        };

        hostLed = led ("HOST LOCK", colours::led, 68.0f);
        midiLed = led ("MIDI IN", colours::amber, 98.0f);
        lfoLed = led ("LFO", juce::Colour (0xffff5a4a), 128.0f);

        setBufferedToImage (true);
        update();
    }

    Knob& ServiceBoard::trimmer (const juce::String& paramId, const juce::String& label, float x, int ticks)
    {
        auto& k = static_cast<Knob&> (*parts.emplace_back (
            std::make_unique<Knob> (processor.getState(), paramId, label, trimmerRadius, ticks, false, true)));
        k.setTrimmerStyle();
        k.place ({ x, trimmerY });
        addAndMakeVisible (k);
        return k;
    }

    void ServiceBoard::update()
    {
        auto& state = processor.getState();
        auto& display = processor.getDisplay();
        const auto now = juce::Time::getMillisecondCounterHiRes();

        if (const auto synced = state.getRawParameterValue (id::lfoSync)->load() > 0.5f; synced != lfoSynced || lfoRate == nullptr)
        {
            lfoSynced = synced;
            lfoRate->bind (state, synced ? id::lfoDivision : id::lfoRate, synced ? 8 : 11);
        }

        if (const auto notes = display.midiNotes.load(); notes != lastMidiNotes)
        {
            lastMidiNotes = notes;
            midiUntil = now + 120.0;
        }

        hostLed->setOn (display.hostLocked.load());
        midiLed->setOn (now < midiUntil);
        lfoLed->setOn (display.lfoHigh.load());
    }

    void ServiceBoard::setLidShadow (float amount)
    {
        if (std::abs (amount - lidShadow) > 1.0e-3f)
        {
            lidShadow = amount;
            repaint();
        }
    }

    void ServiceBoard::mouseUp (const juce::MouseEvent& e)
    {
        for (auto p : Hood::screwPositions (getLocalBounds().toFloat()))
            if (p.getDistanceFrom (e.position) <= 9.0f && onClose)
                onClose();
    }

    void ServiceBoard::paint (juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();

        // the board sits a little lower than the panel: dark gap around it
        g.setColour (juce::Colour (0xff0d140f));
        g.fillRect (bounds);
        const auto board = bounds.reduced (3.0f);
        g.setColour (colours::pcb);
        g.fillRoundedRectangle (board, 4.0f);

        // fine solder-mask texture
        g.setColour (juce::Colours::black.withAlpha (0.06f));
        for (auto y = board.getY() + 4.0f; y < board.getBottom(); y += 6.0f)
            g.drawHorizontalLine ((int) y, board.getX(), board.getRight());

        // copper: from the trimmers down to the chips, and a bus along the bottom
        for (auto x : { 76.0f, 172.0f, 268.0f, 364.0f })
            drawTrace (g, { { x + trimmerRadius + 4.0f, trimmerY }, { x + 30.0f, trimmerY }, { x + 30.0f, 186.0f },
                            { juce::jlimit (90.0f, 330.0f, x + 10.0f), 196.0f } });

        for (auto x : { 500.0f, 596.0f, 692.0f, 788.0f })
            drawTrace (g, { { x + trimmerRadius + 4.0f, trimmerY }, { x + 30.0f, trimmerY }, { x + 30.0f, 186.0f },
                            { juce::jlimit (540.0f, 800.0f, x + 10.0f), 196.0f } });

        drawTrace (g, { { 30.0f, 246.0f }, { 1158.0f, 246.0f } });
        drawTrace (g, { { 880.0f, trimmerY + 30.0f }, { 880.0f, 170.0f }, { 960.0f, 170.0f }, { 960.0f, 128.0f } });
        drawTrace (g, { { 1160.0f, 40.0f }, { 1160.0f, 200.0f }, { 1130.0f, 230.0f } });

        // sections
        drawSection (g, { 20.0f, 34.0f, 412.0f, 140.0f }, "SEQ OPTIONS");
        drawSection (g, { 446.0f, 34.0f, 488.0f, 140.0f }, "LFO");
        drawSection (g, { 948.0f, 34.0f, 220.0f, 140.0f }, "STATUS");

        // parts
        drawChip (g, { 70.0f, 198.0f, 96.0f, 30.0f }, 8, "PS8-SEQ");
        drawChip (g, { 236.0f, 202.0f, 84.0f, 26.0f }, 7, "TL074");
        drawChip (g, { 520.0f, 198.0f, 96.0f, 30.0f }, 8, "LFO-OSC");
        drawChip (g, { 690.0f, 202.0f, 96.0f, 26.0f }, 8, "CD4051");
        drawChip (g, { 990.0f, 194.0f, 120.0f, 34.0f }, 10, "PS8-IO");
        drawCapacitor (g, { 196.0f, 214.0f }, 12.0f);
        drawCapacitor (g, { 440.0f, 214.0f }, 14.0f);
        drawCapacitor (g, { 870.0f, 214.0f }, 12.0f);
        drawResistor (g, { 380.0f, 206.0f }, { 0xff8b4513, 0xff1a1a1a, 0xffd13b2b, 0xffc9a24a });
        drawResistor (g, { 380.0f, 226.0f }, { 0xffe6a12a, 0xff7b4fa0, 0xff8b4513, 0xffc9a24a });
        drawResistor (g, { 830.0f, 206.0f }, { 0xff2f6fb0, 0xff3d8a4a, 0xffd13b2b, 0xffc9a24a });
        drawResistor (g, { 930.0f, 226.0f }, { 0xff8b4513, 0xff1a1a1a, 0xffe6a12a, 0xffc9a24a });

        // silkscreen
        drawLabel (g, "PS-8 SERVICE BOARD  REV 2.1", { 30.0f, 18.0f }, 11.0f, colours::silkscreen, juce::Justification::centredLeft);
        drawLabel (g, "EXTRAS - QUALIFIED PERSONNEL ONLY", { 1150.0f, 18.0f }, 10.0f, colours::silkscreen.withAlpha (0.8f),
                   juce::Justification::centredRight);
        drawLabel (g, "TP1", { 1132.0f, 160.0f }, 9.0f, colours::silkscreen.withAlpha (0.8f));
        g.setColour (juce::Colour (0xffd6b25a));
        g.fillEllipse (juce::Rectangle<float> (8.0f, 8.0f).withCentre ({ 1132.0f, 146.0f }));

        for (auto p : Hood::screwPositions (bounds))
            drawHole (g, p);

        // the open cover throws a shadow
        if (lidShadow > 0.0f)
        {
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.7f * lidShadow), 0.0f, 0.0f,
                                                     juce::Colours::transparentBlack, 0.0f, 90.0f * lidShadow + 10.0f, false));
            g.fillRect (bounds);
        }
    }
}
