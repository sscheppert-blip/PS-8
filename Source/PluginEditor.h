#pragma once

#include "PluginProcessor.h"
#include "ui/Patchbay.h"
#include "ui/ProgramDisplay.h"
#include "ui/Widgets.h"

// The front panel, laid out in design coordinates (designWidth x designHeight) and scaled as a whole.
class FrontPanel final : public juce::Component,
                         private juce::Timer
{
public:
    static constexpr int designWidth = 1200, designHeight = 1016;

    explicit FrontPanel (PS8AudioProcessor&);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void scram();
    void showPage (int page);
    void updateLfoRateKnob (bool synced);

    template <typename T, typename... Args>
    T& make (Args&&... args)
    {
        auto& item = parts.emplace_back (std::make_unique<T> (std::forward<Args> (args)...));
        addAndMakeVisible (*item);
        return static_cast<T&> (*item);
    }

    ps8::ui::Knob& knob (const juce::String& paramId, const juce::String& label, float x, float y, float radius,
                         bool bipolar = false, int ticks = 11, bool showValue = false);
    ps8::ui::Toggle& toggle (const juce::String& paramId, float x, float y, std::vector<float> positions,
                             ps8::ui::Toggle::Labels);

    PS8AudioProcessor& processor;
    std::vector<std::unique_ptr<juce::Component>> parts;

    ps8::ui::Gauge* outputGauge = nullptr;
    ps8::ui::Gauge* loadGauge = nullptr;
    ps8::ui::Nixie* bpmNixie = nullptr;
    ps8::ui::Nixie* stepNixie = nullptr;
    ps8::ui::Lamp* runLamp = nullptr;
    ps8::ui::Lamp* clipLamp = nullptr;
    ps8::ui::Knob* lfoRateKnob = nullptr;
    ps8::ui::Annunciator* hostLamp = nullptr;
    ps8::ui::Annunciator* midiLamp = nullptr;
    ps8::ui::Annunciator* lfoLamp = nullptr;
    ps8::ui::Annunciator* pageLamp = nullptr;
    ps8::ui::Patchbay* patchbay = nullptr;
    ps8::ui::ProgramDisplay* programDisplay = nullptr;

    struct StepColumn
    {
        ps8::ui::Knob* pitch = nullptr;
        ps8::ui::Knob* velocity = nullptr;
        ps8::ui::LampButton* lamp = nullptr;
    };

    std::array<StepColumn, ps8::seq::maxSteps> stepColumns {};
    int shownPage = -1;
    bool lfoSynced = false;
    uint32_t lastMidiNotes = 0;

    float outputNeedle = 0.0f, loadNeedle = 0.0f;
    double clipUntil = 0.0, alarmUntil = 0.0, midiUntil = 0.0;
};

class PS8AudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PS8AudioProcessorEditor (PS8AudioProcessor&);

    void resized() override;

private:
    static constexpr int minWidth = 720, maxWidth = 1800;

    juce::SharedResourcePointer<ps8::UserSettings> settings;
    FrontPanel panel;
    bool sizeRestored = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PS8AudioProcessorEditor)
};
