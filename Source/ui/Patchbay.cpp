#include "Patchbay.h"
#include "Widgets.h"

namespace ps8::ui
{
    namespace
    {
        constexpr float inputX = 62.0f, outputX = 790.0f, columnPitch = 76.0f;
        constexpr int inputColumns = 9;
        constexpr float rowY[] { Patchbay::top + 70.0f, Patchbay::top + 128.0f };
        constexpr float jackRadius = 15.0f;

        juce::Point<float> at (float firstX, int column, int row)
        {
            return { firstX + columnPitch * (float) column, rowY[row] };
        }

        std::array<Patchbay::Jack, Patchbay::numJacks> makeJacks()
        {
            using namespace patch;
            return { {
                // inputs: sound row, then trigger / clock row
                { "VCO 1 CV", false, vco1CvIn,     at (inputX, 0, 0) },
                { "VCO 2 CV", false, vco2CvIn,     at (inputX, 1, 0) },
                { "VCO RST",  false, vcoResetIn,   at (inputX, 2, 0) },
                { "1>2 FM",   false, fmAmountIn,   at (inputX, 3, 0) },
                { "VCO DEC",  false, vcoDecayIn,   at (inputX, 4, 0) },
                { "NOISE",    false, noiseLevelIn, at (inputX, 5, 0) },
                { "EXT IN",   false, extIn,        at (inputX, 6, 0) },
                { "VCF MOD",  false, vcfModIn,     at (inputX, 7, 0) },
                { "VCF DEC",  false, vcfDecayIn,   at (inputX, 8, 0) },
                { "TRIG",     false, trigIn,       at (inputX, 0, 1) },
                { "VELOCITY", false, velocityIn,   at (inputX, 1, 1) },
                { "VCA CV",   false, vcaCvIn,      at (inputX, 2, 1) },
                { "VCA DEC",  false, vcaDecayIn,   at (inputX, 3, 1) },
                { "TEMPO",    false, tempoIn,      at (inputX, 4, 1) },
                { "RUN",      false, runIn,        at (inputX, 5, 1) },
                { "ADV/CLK",  false, advClockIn,   at (inputX, 6, 1) },
                { "RESET",    false, resetIn,      at (inputX, 7, 1) },

                // outputs
                { "VCO 1",    true,  vco1Out,      at (outputX, 0, 0) },
                { "VCO 2",    true,  vco2Out,      at (outputX, 1, 0) },
                { "VCO EG",   true,  vcoEgOut,     at (outputX, 2, 0) },
                { "VCF EG",   true,  vcfEgOut,     at (outputX, 3, 0) },
                { "VCA EG",   true,  vcaEgOut,     at (outputX, 4, 0) },
                { "TRIG",     true,  trigOut,      at (outputX, 0, 1) },
                { "VELOCITY", true,  velocityOut,  at (outputX, 1, 1) },
                { "PITCH",    true,  pitchOut,     at (outputX, 2, 1) },
                { "VCA",      true,  vcaOut,       at (outputX, 3, 1) },
            } };
        }

        // one colour per input, so neighbouring inputs differ
        juce::Colour cableColour (int input)
        {
            static const juce::Colour palette[] {
                juce::Colour (0xffc8322a), juce::Colour (0xffe2b322), juce::Colour (0xff2f6fb0),
                juce::Colour (0xff3d8a4a), juce::Colour (0xffd9772a), juce::Colour (0xffe6e1d0),
                juce::Colour (0xff7b4fa0),
            };

            return palette[input % (int) std::size (palette)];
        }

        void drawPlug (juce::Graphics& g, juce::Point<float> centre, juce::Colour colour)
        {
            const auto body = juce::Rectangle<float> (20.0f, 20.0f).withCentre (centre);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillEllipse (body.translated (1.5f, 2.5f));
            g.setColour (colour.darker (0.5f));
            g.fillEllipse (body);
            g.setColour (colour);
            g.fillEllipse (body.reduced (3.0f));
            g.setColour (colour.brighter (0.6f).withAlpha (0.7f));
            g.fillEllipse (body.reduced (6.0f).translated (-2.0f, -2.0f));
        }

        // a cable sagging under its own weight between two points
        void drawCable (juce::Graphics& g, juce::Point<float> a, juce::Point<float> b, juce::Colour colour)
        {
            // the lowest point of the curve is half way to the control point: keep it on the panel
            constexpr float lowest = Patchbay::top + 186.0f;
            const auto sag = juce::jmin (36.0f + 0.3f * a.getDistanceFrom (b), 2.0f * (lowest - (a.y + b.y) * 0.5f));
            const auto control = (a + b) * 0.5f + juce::Point<float> (0.0f, sag);

            juce::Path path;
            path.startNewSubPath (a);
            path.quadraticTo (control, b);

            g.setColour (juce::Colours::black.withAlpha (0.25f));
            g.strokePath (path, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                          juce::AffineTransform::translation (2.0f, 4.0f));
            g.setColour (colour.darker (0.6f));
            g.strokePath (path, juce::PathStrokeType (6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (colour);
            g.strokePath (path, juce::PathStrokeType (4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (colour.brighter (0.5f).withAlpha (0.45f));
            g.strokePath (path, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded),
                          juce::AffineTransform::translation (-1.0f, -1.2f));

            drawPlug (g, a, colour);
            drawPlug (g, b, colour);
        }
    }

    const std::array<Patchbay::Jack, Patchbay::numJacks>& Patchbay::jacks()
    {
        static const auto all = makeJacks();
        return all;
    }

    const Patchbay::Jack& Patchbay::jackFor (patch::Output o)
    {
        for (const auto& jack : jacks())
            if (jack.output && jack.index == o)
                return jack;

        jassertfalse;
        return jacks()[0];
    }

    const Patchbay::Jack& Patchbay::jackFor (patch::Input in)
    {
        for (const auto& jack : jacks())
            if (! jack.output && jack.index == in)
                return jack;

        jassertfalse;
        return jacks()[0];
    }

    void Patchbay::paintPanel (juce::Graphics& g, float width)
    {
        drawPlate (g, { 20.0f, top, width - 40.0f, height }, "PATCHBAY");

        // divider between inputs and outputs
        const auto dividerX = (inputX + (float) (inputColumns - 1) * columnPitch + outputX) * 0.5f;
        g.setColour (colours::ink.withAlpha (0.5f));
        g.drawLine (dividerX, top + 42.0f, dividerX, top + height - 14.0f, 1.5f);

        for (const auto& jack : jacks())
            drawJack (g, jack.centre, jack.label, jack.output);

        drawLabel (g, "INPUTS", { inputX + 0.5f * (float) (inputColumns - 1) * columnPitch, top + height - 12.0f }, 10.0f, colours::plateShade);
        drawLabel (g, "OUTPUTS", { outputX + 2.0f * columnPitch, top + height - 12.0f }, 10.0f, colours::plateShade);
    }

    //==============================================================================
    Patchbay::Patchbay (PS8AudioProcessor& p) : processor (p)
    {
        setInterceptsMouseClicks (true, false);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        shownVersion = processor.getPatchVersion();
    }

    void Patchbay::update()
    {
        if (const auto version = processor.getPatchVersion(); version != shownVersion)
        {
            shownVersion = version;
            repaint();
        }
    }

    const Patchbay::Jack* Patchbay::jackAt (juce::Point<float> position) const
    {
        for (const auto& jack : jacks())
            if (jack.centre.getDistanceFrom (position) <= jackRadius)
                return &jack;

        return nullptr;
    }

    bool Patchbay::hitTest (int x, int y)
    {
        return dragging || jackAt ({ (float) x, (float) y }) != nullptr;
    }

    void Patchbay::paint (juce::Graphics& g)
    {
        const auto masks = processor.getPatch();

        for (int in = 0; in < patch::numInputs; ++in)
            for (int o = 0; o < patch::numOutputs; ++o)
                if ((masks[(size_t) in] >> o) & 1u)
                    drawCable (g, jackFor ((patch::Output) o).centre, jackFor ((patch::Input) in).centre, cableColour (in));

        if (dragging && fixedEnd != nullptr)
        {
            // light up the jacks the loose plug fits into
            for (const auto& jack : jacks())
            {
                if (jack.output != fixedEnd->output)
                {
                    g.setColour (colours::amber.withAlpha (jack.centre.getDistanceFrom (loosePlug) <= jackRadius ? 0.95f : 0.45f));
                    g.drawEllipse (juce::Rectangle<float> (30.0f, 30.0f).withCentre (jack.centre), 2.5f);
                }
            }

            drawCable (g, fixedEnd->centre, loosePlug, dragColour);
        }
    }

    void Patchbay::mouseDown (const juce::MouseEvent& e)
    {
        dragging = false;
        const auto* jack = jackAt (e.position);

        if (jack == nullptr)
            return;

        if (e.mods.isPopupMenu())
        {
            pullCables (*jack);
            return;
        }

        fixedEnd = jack;
        dragColour = jack->output ? colours::plateText : cableColour (jack->index);

        // pulling a plug out of an input picks up its newest cable, still hanging from the output
        if (! jack->output)
        {
            const auto mask = processor.getPatch()[(size_t) jack->index];

            for (int o = patch::numOutputs; --o >= 0;)
            {
                if ((mask >> o) & 1u)
                {
                    processor.disconnect ((patch::Output) o, (patch::Input) jack->index);
                    fixedEnd = &jackFor ((patch::Output) o);
                    dragColour = cableColour (jack->index);
                    break;
                }
            }
        }

        dragging = true;
        loosePlug = e.position;
        repaint();
    }

    void Patchbay::mouseDrag (const juce::MouseEvent& e)
    {
        if (dragging)
        {
            loosePlug = e.position;
            repaint();
        }
    }

    void Patchbay::mouseUp (const juce::MouseEvent& e)
    {
        if (! dragging)
            return;

        dragging = false;

        if (const auto* target = jackAt (e.position); target != nullptr && fixedEnd != nullptr && target->output != fixedEnd->output)
        {
            const auto& out = fixedEnd->output ? *fixedEnd : *target;
            const auto& in = fixedEnd->output ? *target : *fixedEnd;
            processor.connect ((patch::Output) out.index, (patch::Input) in.index);
        }

        fixedEnd = nullptr;
        update();
        repaint();
    }

    void Patchbay::mouseDoubleClick (const juce::MouseEvent& e)
    {
        dragging = false;
        fixedEnd = nullptr;

        if (const auto* jack = jackAt (e.position))
            pullCables (*jack);
    }

    void Patchbay::pullCables (const Jack& jack)
    {
        for (int other = 0; other < (jack.output ? patch::numInputs : patch::numOutputs); ++other)
        {
            const auto o = (patch::Output) (jack.output ? jack.index : other);
            const auto in = (patch::Input) (jack.output ? other : jack.index);

            if (processor.isConnected (o, in))
                processor.disconnect (o, in);
        }

        update();
        repaint();
    }
}
