#include "StatusDisplay.h"
#include "Parameters.h"
#include "Theme.h"

namespace ps8::ui
{
    namespace
    {
        namespace id = params::id;

        constexpr double touchTime = 1500.0;   // ms a touched control stays on the display
        constexpr int maxTouchedAtOnce = 3;    // more changes at once = a preset or pattern load

        const juce::Colour bright { 0xffd9f0ff };

        juce::Font mono (float height)
        {
            return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
        }
    }

    StatusDisplay::StatusDisplay (PS8AudioProcessor& p) : processor (p)
    {
        setInterceptsMouseClicks (false, false);

        // view state is not worth a message
        for (auto* parameter : processor.getParameters())
            if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (parameter))
                if (const auto& pid = ranged->getParameterID(); pid != id::viewPage && pid != id::serviceOpen && pid != id::stepRow)
                    watched.push_back ({ ranged, ranged->getValue() });

        update();
    }

    void StatusDisplay::update()
    {
        const auto now = juce::Time::getMillisecondCounterHiRes();
        juce::RangedAudioParameter* changed = nullptr;
        int numChanged = 0;

        for (auto& w : watched)
        {
            if (const auto v = w.parameter->getValue(); v != w.value)
            {
                w.value = v;
                changed = w.parameter;
                ++numChanged;
            }
        }

        if (numChanged > 0 && numChanged <= maxTouchedAtOnce)
        {
            touched = changed;
            touchedUntil = now + touchTime;
        }
        else if (numChanged > maxTouchedAtOnce || now > touchedUntil)
        {
            touched = nullptr;
        }

        // repaint only when what is shown changes
        auto& state = processor.getState();
        auto& display = processor.getDisplay();
        juce::String shown;
        shown << processor.getPatterns().current() << '|' << (int) display.bpm.load() << '|' << display.step.load() << '|'
                 << (int) display.running.load() << '|' << state.getRawParameterValue (id::hostClock)->load() << '|'
                 << state.getRawParameterValue (id::length)->load() << '|' << processor.getPresets().current().name
                 << (int) processor.getPresets().isModified() << '|';

        for (int i = 0; i < seq::maxSteps; ++i)
            shown << (state.getRawParameterValue (id::stepOn (i))->load() > 0.5f ? '1' : '0');

        if (touched != nullptr)
            shown << '|' << touched->getName (32) << touched->getCurrentValueAsText();

        if (shown != text)
        {
            text = shown;
            repaint();
        }
    }

    void StatusDisplay::paint (juce::Graphics& g)
    {
        auto& state = processor.getState();
        auto& display = processor.getDisplay();

        // bezel and glass
        const auto bezel = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff050506));
        g.fillRoundedRectangle (bezel, 8.0f);
        g.setColour (juce::Colour (0xff2c2d30));
        g.drawRoundedRectangle (bezel.reduced (0.5f), 8.0f, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine (1, 8.0f, bezel.getRight() - 8.0f);

        const auto screen = bezel.reduced (8.0f);
        g.setColour (juce::Colour (0xff020304));
        g.fillRoundedRectangle (screen, 4.0f);

        const auto inner = screen.reduced (12.0f, 7.0f);
        const auto dim = matte::oled.withAlpha (0.85f);

        // top row: pattern, clock, tempo
        const auto hostClock = state.getRawParameterValue (id::hostClock)->load() > 0.5f;
        auto row = inner.withHeight (13.0f);
        g.setFont (mono (11.0f));
        g.setColour (dim);
        g.drawText ("PATTERN " + juce::String (processor.getPatterns().current() + 1), row, juce::Justification::centredLeft, false);
        g.drawText (hostClock ? "HOST" : "INT", row, juce::Justification::centred, false);
        g.drawText (juce::String (juce::roundToInt (display.bpm.load())) + " BPM", row, juce::Justification::centredRight, false);

        // big line: the control just touched, else the preset
        const auto big = inner.withTrimmedTop (16.0f).withHeight (28.0f);
        g.setColour (bright);

        if (touched != nullptr)
        {
            g.setFont (mono (19.0f));
            g.drawText (touched->getName (24).toUpperCase(), big, juce::Justification::centredLeft, true);
            g.drawText (touched->getCurrentValueAsText().toUpperCase(), big, juce::Justification::centredRight, false);
        }
        else
        {
            auto& presets = processor.getPresets();
            g.setFont (mono (22.0f));
            g.drawText (presets.current().name.toUpperCase() + (presets.isModified() ? " *" : ""), big,
                        juce::Justification::centredLeft, true);
        }

        // step and transport
        const auto step = display.step.load();
        const auto length = juce::roundToInt (state.getRawParameterValue (id::length)->load());
        row = inner.withTrimmedTop (45.0f).withHeight (13.0f);
        g.setFont (mono (11.0f));
        g.setColour (dim);
        g.drawText ("STEP " + (step >= 0 ? juce::String (step + 1).paddedLeft ('0', 2) : juce::String ("--")) + "/"
                        + juce::String (length).paddedLeft ('0', 2),
                    row, juce::Justification::centredLeft, false);
        g.drawText (display.running.load() ? "PLAY" : "STOP", row, juce::Justification::centredRight, false);

        // the sequence
        const auto bar = inner.withTop (inner.getBottom() - 8.0f);
        const auto pitch = (bar.getWidth() + 3.0f) / (float) seq::maxSteps;

        for (int i = 0; i < seq::maxSteps; ++i)
        {
            const auto cell = juce::Rectangle<float> (bar.getX() + pitch * (float) i, bar.getY(), pitch - 3.0f, bar.getHeight());
            const auto on = state.getRawParameterValue (id::stepOn (i))->load() > 0.5f;
            const auto inRange = i < length;

            if (i == step)
            {
                g.setColour (juce::Colours::white.withAlpha (0.3f));
                g.fillRoundedRectangle (cell.expanded (2.0f), 2.0f);
                g.setColour (juce::Colours::white);
                g.fillRoundedRectangle (cell, 1.0f);
            }
            else if (on && inRange)
            {
                g.setColour (matte::oled);
                g.fillRoundedRectangle (cell, 1.0f);
            }
            else
            {
                g.setColour (matte::oled.withAlpha (inRange ? 0.5f : 0.15f));
                g.drawRoundedRectangle (cell.reduced (0.5f), 1.0f, 1.0f);
            }
        }

        // glass reflection
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), screen.getX(), screen.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), screen.getX() + screen.getWidth() * 0.35f,
                                                 screen.getBottom(), false));
        g.fillRoundedRectangle (screen, 4.0f);
    }
}
