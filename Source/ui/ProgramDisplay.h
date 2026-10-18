#pragma once

#include "PluginProcessor.h"
#include "Presets.h"

// Program display in the name plate: the preset name (with * when changed), < > to step through
// the presets, SAVE and MENU. A click on the name opens the menu as well.
namespace ps8::ui
{
    class ProgramDisplay final : public juce::Component
    {
    public:
        static constexpr int width = 176, height = 84;

        explicit ProgramDisplay (PS8AudioProcessor&);

        // call regularly; repaints when the name or the modified state changes
        void update();

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

        // the actions behind the keys (public for the tests)
        void showMenu();
        void askToSave();

    private:
        enum class Key { none, previous, next, save, menu, name };

        Key keyAt (juce::Point<float>) const;
        void paintMatte (juce::Graphics&);
        juce::Rectangle<float> areaOf (Key) const;
        void saveAs (const juce::String& name);
        void askToRename();
        void askToDelete();

        PS8AudioProcessor& processor;
        Presets& presets;
        juce::SharedResourcePointer<UserSettings> settings;
        Key pressed = Key::none;
        juce::String shownName;
        bool shownModified = false, shownFactory = true;
    };
}
