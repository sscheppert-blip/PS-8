#pragma once

#include "PluginProcessor.h"
#include "ui/Patchbay.h"
#include "ui/ProgramDisplay.h"
#include "ui/ServiceBoard.h"
#include "ui/Widgets.h"

// The front panel, laid out in design coordinates (designWidth x designHeight) and scaled as a whole.
class FrontPanel final : public juce::Component,
                         private juce::Timer
{
public:
    static constexpr int designWidth = 1200, designHeight = 876;

    explicit FrontPanel (PS8AudioProcessor&);

    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void scram();
    void showPage (int page);
    void setService (bool open);
    void animateHatch();
    void paintModules (juce::Graphics&) const;

    template <typename T, typename... Args>
    T& make (Args&&... args)
    {
        return makeIn<T> (*this, std::forward<Args> (args)...);
    }

    template <typename T, typename... Args>
    T& makeIn (juce::Component& parent, Args&&... args)
    {
        auto& item = parts.emplace_back (std::make_unique<T> (std::forward<Args> (args)...));
        parent.addAndMakeVisible (*item);
        return static_cast<T&> (*item);
    }

    // controls on the module row live on the hood; x/y are panel coordinates
    ps8::ui::Knob& knob (const juce::String& paramId, const juce::String& label, float x, float y, float radius,
                         bool bipolar = false, int ticks = 11, bool showValue = false, juce::Component* parent = nullptr);
    ps8::ui::Toggle& toggle (const juce::String& paramId, float x, float y, std::vector<float> positions,
                             ps8::ui::Toggle::Labels, juce::Component* parent = nullptr);

    PS8AudioProcessor& processor;
    std::vector<std::unique_ptr<juce::Component>> parts;

    ps8::ui::Gauge* outputGauge = nullptr;
    ps8::ui::Gauge* loadGauge = nullptr;
    ps8::ui::Nixie* bpmNixie = nullptr;
    ps8::ui::Nixie* stepNixie = nullptr;
    ps8::ui::Lamp* runLamp = nullptr;
    ps8::ui::Lamp* clipLamp = nullptr;
    ps8::ui::Hood* hood = nullptr;
    ps8::ui::ServiceBoard* board = nullptr;
    ps8::ui::PageTabs* pageTabs = nullptr;
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

    // service hatch: screws 0..1 turn out, then the lid 0..1 flips up
    struct Animator final : juce::Timer
    {
        std::function<void()> tick;
        void timerCallback() override { tick(); }
    } animator;

    bool serviceOpen = false;
    float screws = 0.0f, lid = 0.0f;
    double lastFrame = 0.0;

    float outputNeedle = 0.0f, loadNeedle = 0.0f;
    double clipUntil = 0.0, alarmUntil = 0.0;
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
