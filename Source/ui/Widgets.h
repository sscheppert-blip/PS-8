#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Control-room style front panel parts: bakelite knobs, chrome toggle switches, lamp buttons,
// nixie tubes, needle gauges and the SCRAM button.
namespace ps8::ui
{
    namespace colours
    {
        inline const juce::Colour frame      { 0xff5f7368 };
        inline const juce::Colour panel      { 0xff8ea596 };
        inline const juce::Colour plate      { 0xffa3b8aa };
        inline const juce::Colour plateShade { 0xff4f6157 };
        inline const juce::Colour ink        { 0xff1f2a23 };
        inline const juce::Colour plateText  { 0xffd9e3d4 };
        inline const juce::Colour dimText    { 0xff9db0a2 };
        inline const juce::Colour hazard     { 0xffe2b322 };
        inline const juce::Colour amber      { 0xffffb030 };
        inline const juce::Colour red        { 0xffc42318 };
        inline const juce::Colour nixie      { 0xffff9a2e };
    }

    juce::Font stencilFont (float height);
    juce::Font labelFont (float height);

    void drawScrew (juce::Graphics&, juce::Point<float> centre);
    void drawPlate (juce::Graphics&, juce::Rectangle<float> area, const juce::String& title);
    void drawHazard (juce::Graphics&, juce::Rectangle<float> area);
    void drawChrome (juce::Graphics&, juce::Point<float> centre, float radius);
    void drawJack (juce::Graphics&, juce::Point<float> centre, const juce::String& label, bool output);
    void drawLabel (juce::Graphics&, const juce::String& text, juce::Point<float> centre, float height,
                    juce::Colour = colours::ink, juce::Justification = juce::Justification::centred);

    //==============================================================================
    class KnobLookAndFeel final : public juce::LookAndFeel_V4
    {
    public:
        KnobLookAndFeel();

        void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                               float startAngle, float endAngle, juce::Slider&) override;
        juce::Font getSliderPopupFont (juce::Slider&) override { return labelFont (14.0f); }
        void drawBubble (juce::Graphics&, juce::BubbleComponent&, const juce::Point<float>& tip,
                         const juce::Rectangle<float>& body) override;
    };

    // Rotary knob with a printed scale and its label underneath (optionally with a small value readout).
    // Placed by its centre.
    class Knob final : public juce::Component
    {
    public:
        Knob (juce::AudioProcessorValueTreeState&, const juce::String& paramId, const juce::String& label,
              float radius, int numTicks = 11, bool bipolar = false, bool showValue = false);
        ~Knob() override;

        // attach to another parameter (e.g. LFO rate free/synced)
        void bind (juce::AudioProcessorValueTreeState&, const juce::String& paramId, int numTicks);

        void place (juce::Point<float> centre);
        void paint (juce::Graphics&) override;
        void resized() override;
        void parentHierarchyChanged() override;

    private:
        juce::SharedResourcePointer<KnobLookAndFeel> lookAndFeel;
        juce::Slider slider;
        juce::String label;
        float radius;
        bool showValue;
        juce::RangedAudioParameter* parameter = nullptr;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    // Two- or three-position toggle switch bound to a parameter. positions[i] is the parameter value
    // for lever position i, from top to bottom.
    class Toggle final : public juce::Component
    {
    public:
        struct Labels
        {
            Labels (juce::String topText, juce::String bottomText, juce::String nameText = {},
                    juce::String middleText = {}, bool isNameAbove = false)
                : top (std::move (topText)), bottom (std::move (bottomText)), name (std::move (nameText)),
                  middle (std::move (middleText)), nameAbove (isNameAbove) {}

            juce::String top, bottom, name, middle;
            bool nameAbove = false;
        };

        Toggle (juce::AudioProcessorValueTreeState&, const juce::String& paramId, std::vector<float> positions,
                Labels);

        void place (juce::Point<float> centre);
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        void setIndex (int index);
        juce::Point<float> centre() const;

        std::vector<float> positions;
        Labels labels;
        int index = 0;
        juce::ParameterAttachment attachment;
    };

    // Square lamp push button of a sequencer step: toggles the step on/off, lights up when playing.
    class LampButton final : public juce::Component
    {
    public:
        LampButton (juce::AudioProcessorValueTreeState&, const juce::String& paramId);

        void place (juce::Point<float> centre);
        void setState (bool isCurrent, bool alarm, bool inRange);
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        bool on = true, current = false, alarm = false, inRange = true;
        juce::ParameterAttachment attachment;
    };

    // Round push button (yellow cap in a chrome bezel), or the big red SCRAM button.
    class PushButton final : public juce::Component
    {
    public:
        PushButton (const juce::String& label, bool scram = false);

        void place (juce::Point<float> centre, float radius);
        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

        std::function<void()> onPress;

    private:
        juce::String label;
        bool scram, down = false;
        float radius = 10.0f;
    };

    class Nixie final : public juce::Component
    {
    public:
        explicit Nixie (int numDigits);

        void setText (const juce::String&);
        void paint (juce::Graphics&) override;

        static constexpr float tubeWidth = 30.0f, tubeHeight = 50.0f, tubePitch = 34.0f;

    private:
        int numDigits;
        juce::String text;
    };

    // Needle instrument; value 0..1 sweeps the scale.
    class Gauge final : public juce::Component
    {
    public:
        Gauge (const juce::String& label, juce::StringArray marks);

        void setValue (float);
        void paint (juce::Graphics&) override;

    private:
        juce::String label;
        juce::StringArray marks;
        float value = 0.0f;
    };

    // Back-lit annunciator tile with an engraved caption.
    class Annunciator final : public juce::Component
    {
    public:
        Annunciator (const juce::String& caption, juce::Colour onColour);

        void setOn (bool);
        void paint (juce::Graphics&) override;

    private:
        juce::String caption;
        juce::Colour onColour;
        bool on = false;
    };

    class Lamp final : public juce::Component
    {
    public:
        Lamp (juce::Colour onColour, juce::Colour offColour);

        void setOn (bool);
        void paint (juce::Graphics&) override;

    private:
        juce::Colour onColour, offColour;
        bool on = false;
    };
}
