#pragma once

#include "PluginProcessor.h"
#include "Theme.h"

// The patchbay strip: jack layout, the cables drawn over the whole panel, and plugging by mouse.
// Drag from a jack to another to patch; dragging a plug out of an input moves or removes that cable;
// double-click or right-click a jack to pull all its cables. Lives in panel design coordinates.
namespace ps8::ui
{
    class Patchbay final : public juce::Component
    {
    public:
        struct Jack
        {
            const char* label;
            bool output;
            int index;                   // patch::Output or patch::Input
            juce::Point<float> centre;
        };

        static constexpr int numJacks = (int) patch::numOutputs + (int) patch::numInputs;
        static constexpr float top = 664.0f, height = 168.0f;

        static const std::array<Jack, numJacks>& jacks();
        static const Jack& jackFor (patch::Output);
        static const Jack& jackFor (patch::Input);
        // centre of the knob of attenuator 0 or 1 (between its IN and OUT jacks)
        static juce::Point<float> attenuatorKnob (int index);

        // plate, labels and empty jacks (part of the panel background)
        static void paintPanel (juce::Graphics&, float width, Theme);

        explicit Patchbay (PS8AudioProcessor&);

        // call regularly; repaints when the patch has changed (also after loading a state)
        void update();

        bool hitTest (int x, int y) override;
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;

    private:
        const Jack* jackAt (juce::Point<float>) const;
        void pullCables (const Jack&);

        PS8AudioProcessor& processor;
        uint32_t shownVersion = 0;

        const Jack* fixedEnd = nullptr;   // the jack the dragged cable hangs from
        juce::Point<float> loosePlug;
        juce::Colour dragColour;
        bool dragging = false;
    };
}
