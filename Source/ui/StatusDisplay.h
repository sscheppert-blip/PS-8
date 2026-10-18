#pragma once

#include "PluginProcessor.h"

// Matte black: the OLED status display in the top row. Shows pattern, clock and tempo, the preset
// name (or, for a moment, the control just touched with its value), the step and a bar of the
// sequence (lit = step on, white = playing).
namespace ps8::ui
{
    class StatusDisplay final : public juce::Component
    {
    public:
        explicit StatusDisplay (PS8AudioProcessor&);

        // call regularly (about 30 Hz)
        void update();

        void paint (juce::Graphics&) override;

    private:
        struct Watched
        {
            juce::RangedAudioParameter* parameter;
            float value;
        };

        PS8AudioProcessor& processor;
        std::vector<Watched> watched;

        juce::RangedAudioParameter* touched = nullptr;
        double touchedUntil = 0.0;
        juce::String text;   // everything shown, to repaint only on changes
    };
}
