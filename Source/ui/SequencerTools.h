#pragma once

#include "PluginProcessor.h"

// In the sequencer's title bar: PATTERN keys 1–8 (click = play that pattern, right-click = copy to /
// clear) and the step tools – shift left/right, COPY/PASTE the shown page, RAND (menu: what to
// randomise) and UNDO for the last tool.
namespace ps8::ui
{
    class SequencerTools final : public juce::Component
    {
    public:
        explicit SequencerTools (PS8AudioProcessor&);

        // call regularly; repaints when the patterns or the tool states change
        void update();

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

        // x where the tools start (the title sits between the pattern keys and the tools)
        static constexpr float patternsEnd = 256.0f, toolsStart = 488.0f;
        static constexpr int width = 736, height = 22;

        enum class Key { none, pattern, shiftLeft, shiftRight, copy, paste, randomise, undo };

        // the actions behind the keys (public for the tests)
        void press (Key, int pattern = 0);
        void showPatternMenu (int pattern);
        void showRandomiseMenu();

    private:
        struct Hit
        {
            Key key = Key::none;
            int pattern = 0;
        };

        Hit hitAt (juce::Point<float>) const;
        juce::Rectangle<float> patternKey (int index) const;
        juce::Rectangle<float> toolKey (Key) const;
        int shownPage() const;

        PS8AudioProcessor& processor;
        Patterns& patterns;
        Hit pressed;

        int shownCurrent = -1;
        uint32_t shownContent = 0;
        bool shownPaste = false, shownUndo = false;
        double copiedUntil = 0.0;
    };
}
