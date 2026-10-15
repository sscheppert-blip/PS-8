#pragma once

#include "PluginProcessor.h"
#include "Widgets.h"

// The service hatch: the module row is a cover held by four screws ("Hood"). Unscrewed and flipped
// up, it reveals a green circuit board ("ServiceBoard") with the extras that are not part of the
// original instrument: sequencer options, LFO and status LEDs, set with trimmers and a jumper.
namespace ps8::ui
{
    // Two-position jumper on a three-pin header, bound to a bool parameter (top = off, bottom = on).
    class Jumper final : public juce::Component
    {
    public:
        Jumper (juce::AudioProcessorValueTreeState&, const juce::String& paramId,
                juce::String topLabel, juce::String bottomLabel, juce::String name);

        void place (juce::Point<float> centre);
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        juce::String topLabel, bottomLabel, name;
        bool on = false;
        juce::ParameterAttachment attachment;
    };

    // Small board LED with a silkscreen caption on its right.
    class Led final : public juce::Component
    {
    public:
        Led (juce::String caption, juce::Colour colour);

        void setOn (bool);
        void paint (juce::Graphics&) override;

    private:
        juce::String caption;
        juce::Colour colour;
        bool on = false;
    };

    // Key switch "SERVICE" that opens and closes the hatch.
    class KeySwitch final : public juce::Component
    {
    public:
        KeySwitch (juce::AudioProcessorValueTreeState&, const juce::String& paramId);

        void place (juce::Point<float> centre);
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        bool open = false;
        juce::ParameterAttachment attachment;
    };

    // Page tabs "1-8" / "9-16" in the sequencer's title bar; a dot marks the page that is playing.
    class PageTabs final : public juce::Component
    {
    public:
        PageTabs (juce::AudioProcessorValueTreeState&, const juce::String& paramId);

        void setPlayingPage (int page);
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        int shown = 0, playing = -1;
        juce::ParameterAttachment attachment;
    };

    // The cover over the module row. Draws the module plates (via painter, in panel coordinates),
    // four screws that turn out while opening, and darkens while it flips up.
    class Hood final : public juce::Component
    {
    public:
        Hood();

        std::function<void (juce::Graphics&)> painter;
        std::function<void()> onUnscrew;

        // 0 = screws in, 1 = screws out; shade 0..1 darkens the flipping cover
        void setScrews (float progress);
        void setShade (float amount);

        static std::array<juce::Point<float>, 4> screwPositions (juce::Rectangle<float> area);

        void paint (juce::Graphics&) override;
        bool hitTest (int x, int y) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        float screws = 0.0f, shade = 0.0f;
    };

    class ServiceBoard final : public juce::Component
    {
    public:
        explicit ServiceBoard (PS8AudioProcessor&);

        std::function<void()> onClose;

        // call regularly: LEDs and the LFO rate trimmer (free / synced)
        void update();
        void setLidShadow (float amount);

        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        Knob& trimmer (const juce::String& paramId, const juce::String& label, float x, int ticks);

        PS8AudioProcessor& processor;
        std::vector<std::unique_ptr<juce::Component>> parts;
        Knob* lfoRate = nullptr;
        Led* hostLed = nullptr;
        Led* midiLed = nullptr;
        Led* lfoLed = nullptr;
        bool lfoSynced = false;
        uint32_t lastMidiNotes = 0;
        double midiUntil = 0.0;
        float lidShadow = 0.0f;
    };
}
