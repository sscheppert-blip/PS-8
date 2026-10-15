#include "SequencerTools.h"
#include "Parameters.h"
#include "Widgets.h"

namespace ps8::ui
{
    namespace
    {
        constexpr float keyWidth = 21.0f, keyGap = 3.0f, firstPatternX = 60.0f;

        // RAND menu items
        constexpr std::pair<int, const char*> randomiseItems[] {
            { 1, "Pitch" }, { 2, "Velocity" }, { 3, "Ratchet" }, { 4, "Chance" }, { 5, "Steps on/off" }
        };
        constexpr int randomiseEverything = 6;
    }

    SequencerTools::SequencerTools (PS8AudioProcessor& p) : processor (p), patterns (p.getPatterns())
    {
        setSize (width, height);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        update();
    }

    int SequencerTools::shownPage() const
    {
        return processor.getState().getRawParameterValue (params::id::viewPage)->load() > 0.5f ? 1 : 0;
    }

    juce::Rectangle<float> SequencerTools::patternKey (int index) const
    {
        return { firstPatternX + (keyWidth + keyGap) * (float) index, 2.0f, keyWidth, (float) height - 4.0f };
    }

    juce::Rectangle<float> SequencerTools::toolKey (Key key) const
    {
        const auto h = (float) height - 4.0f;

        switch (key)
        {
            case Key::shiftLeft:  return { toolsStart, 2.0f, 22.0f, h };
            case Key::shiftRight: return { toolsStart + 26.0f, 2.0f, 22.0f, h };
            case Key::copy:       return { toolsStart + 54.0f, 2.0f, 44.0f, h };
            case Key::paste:      return { toolsStart + 102.0f, 2.0f, 46.0f, h };
            case Key::randomise:  return { toolsStart + 152.0f, 2.0f, 44.0f, h };
            case Key::undo:       return { toolsStart + 200.0f, 2.0f, 44.0f, h };
            case Key::pattern:
            case Key::none:       break;
        }

        return {};
    }

    SequencerTools::Hit SequencerTools::hitAt (juce::Point<float> position) const
    {
        for (int i = 0; i < Patterns::numPatterns; ++i)
            if (patternKey (i).contains (position))
                return { Key::pattern, i };

        for (const auto key : { Key::shiftLeft, Key::shiftRight, Key::copy, Key::paste, Key::randomise, Key::undo })
            if (toolKey (key).contains (position))
                return { key, 0 };

        return {};
    }

    void SequencerTools::update()
    {
        uint32_t content = 0;

        for (int i = 0; i < Patterns::numPatterns; ++i)
            if (patterns.hasContent (i))
                content |= 1u << i;

        const auto current = patterns.current();
        const auto paste = patterns.canPaste();
        const auto undo = patterns.canUndo();

        if (current != shownCurrent || content != shownContent || paste != shownPaste || undo != shownUndo
            || (copiedUntil > 0.0 && juce::Time::getMillisecondCounterHiRes() > copiedUntil))
        {
            shownCurrent = current;
            shownContent = content;
            shownPaste = paste;
            shownUndo = undo;

            if (juce::Time::getMillisecondCounterHiRes() > copiedUntil)
                copiedUntil = 0.0;

            repaint();
        }
    }

    void SequencerTools::paint (juce::Graphics& g)
    {
        drawLabel (g, "PATTERN", { 2.0f, (float) height * 0.5f }, 10.0f, colours::dimText, juce::Justification::centredLeft);

        auto key = [&g] (juce::Rectangle<float> area, bool lit, bool down)
        {
            g.setColour (lit ? colours::plateText : down ? colours::ink.darker (0.3f) : colours::ink.brighter (0.25f));
            g.fillRoundedRectangle (area.translated (0.0f, down ? 1.0f : 0.0f), 2.0f);
            g.setColour (colours::plateText.withAlpha (0.6f));
            g.drawRoundedRectangle (area, 2.0f, 1.0f);
        };

        for (int i = 0; i < Patterns::numPatterns; ++i)
        {
            const auto area = patternKey (i);
            const auto lit = i == shownCurrent;
            const auto filled = ((shownContent >> i) & 1u) != 0;
            key (area, lit, pressed.key == Key::pattern && pressed.pattern == i);

            // number: dark on the lit key, amber for a pattern with steps in it, dim when empty
            g.setColour (lit ? colours::ink : filled ? colours::nixie : colours::dimText.withAlpha (0.7f));
            g.setFont (stencilFont (13.0f));
            g.drawText (juce::String (i + 1), area.translated (0.0f, 0.5f), juce::Justification::centred, false);
        }

        const auto copied = copiedUntil > 0.0;

        for (const auto k : { Key::shiftLeft, Key::shiftRight, Key::copy, Key::paste, Key::randomise, Key::undo })
        {
            const auto area = toolKey (k);
            const auto enabled = (k != Key::paste || shownPaste) && (k != Key::undo || shownUndo);
            key (area, k == Key::copy && copied, pressed.key == k);
            const auto ink = k == Key::copy && copied ? colours::ink : enabled ? colours::plateText : colours::plateText.withAlpha (0.3f);

            if (k == Key::shiftLeft || k == Key::shiftRight)
            {
                // two small arrows: the steps move this way
                const auto d = k == Key::shiftRight ? 1.0f : -1.0f;
                const auto c = area.getCentre();
                g.setColour (ink);

                for (const auto offset : { -3.0f, 3.0f })
                {
                    juce::Path p;
                    p.addTriangle (c.x + offset + 3.0f * d, c.y, c.x + offset - 2.5f * d, c.y - 4.0f, c.x + offset - 2.5f * d, c.y + 4.0f);
                    g.fillPath (p);
                }
            }
            else
            {
                const char* text = k == Key::copy ? "COPY" : k == Key::paste ? "PASTE" : k == Key::randomise ? "RAND" : "UNDO";
                drawLabel (g, text, area.getCentre(), 9.5f, ink);
            }
        }
    }

    void SequencerTools::mouseDown (const juce::MouseEvent& e)
    {
        pressed = hitAt (e.position);

        // right-click on a pattern key: copy to / clear
        if (e.mods.isPopupMenu() && pressed.key == Key::pattern)
        {
            const auto index = pressed.pattern;
            pressed = {};
            repaint();
            showPatternMenu (index);
            return;
        }

        repaint();
    }

    void SequencerTools::mouseUp (const juce::MouseEvent& e)
    {
        const auto hit = pressed;
        pressed = {};
        repaint();

        const auto now = hitAt (e.position);

        if (hit.key != Key::none && now.key == hit.key && now.pattern == hit.pattern)
            press (hit.key, hit.pattern);
    }

    void SequencerTools::press (Key key, int pattern)
    {
        switch (key)
        {
            case Key::pattern:    patterns.select (pattern); break;
            case Key::shiftLeft:  patterns.shift (-1); break;
            case Key::shiftRight: patterns.shift (1); break;
            case Key::paste:      patterns.pastePage (shownPage()); break;
            case Key::undo:       patterns.undo(); break;
            case Key::randomise:  showRandomiseMenu(); break;
            case Key::copy:
                patterns.copyPage (shownPage());
                copiedUntil = juce::Time::getMillisecondCounterHiRes() + 400.0;
                break;
            case Key::none:       break;
        }

        update();
        repaint();
    }

    void SequencerTools::showPatternMenu (int pattern)
    {
        juce::PopupMenu copyTo;

        for (int i = 0; i < Patterns::numPatterns; ++i)
            copyTo.addItem (100 + i, "Pattern " + juce::String (i + 1), i != pattern);

        juce::PopupMenu menu;
        menu.addSectionHeader ("PATTERN " + juce::String (pattern + 1));
        menu.addSubMenu ("Copy to", copyTo);
        menu.addItem (1, "Clear", patterns.hasContent (pattern));

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [safe = juce::Component::SafePointer<SequencerTools> (this), pattern] (int result)
        {
            if (safe == nullptr || result == 0)
                return;

            if (result == 1)
                safe->patterns.clear (pattern);
            else if (result >= 100)
                safe->patterns.copy (pattern, result - 100);

            safe->update();
        });
    }

    void SequencerTools::showRandomiseMenu()
    {
        juce::PopupMenu menu;
        menu.addSectionHeader ("RANDOMISE (STEPS IN LENGTH)");

        for (const auto& [itemId, name] : randomiseItems)
            menu.addItem (itemId, name);

        menu.addSeparator();
        menu.addItem (randomiseEverything, "Everything");

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [safe = juce::Component::SafePointer<SequencerTools> (this)] (int result)
        {
            if (safe == nullptr || result == 0)
                return;

            static const Patterns::Randomise what[] { Patterns::Randomise::pitch, Patterns::Randomise::velocity,
                                                      Patterns::Randomise::ratchet, Patterns::Randomise::chance,
                                                      Patterns::Randomise::steps, Patterns::Randomise::everything };
            juce::Random random;
            safe->patterns.randomise (what[juce::jlimit (1, randomiseEverything, result) - 1], random);
            safe->update();
        });
    }
}
