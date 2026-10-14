#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <clap-juce-extensions/clap-juce-extensions.h>

#include "dsp/Engine.h"
#include "dsp/Sequencer.h"

class PS8AudioProcessor final : public juce::AudioProcessor,
                                public clap_juce_extensions::clap_properties
{
public:
    // MIDI notes (any channel)
    static constexpr int firstStepNote = 36;   // C1..D#2 play steps 1..16
    static constexpr int advanceNote   = 52;   // E2  next step
    static constexpr int triggerNote   = 53;   // F2  current step again
    static constexpr int resetNote     = 54;   // F#2 back to step 1

    PS8AudioProcessor();
    ~PS8AudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Init"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Front panel interface
    juce::AudioProcessorValueTreeState& getState() noexcept { return state; }

    void requestReset() noexcept   { resetRequest = true; }
    void requestAdvance() noexcept { advanceRequest = true; }
    void requestTrigger() noexcept { triggerRequest = true; }
    void requestScram() noexcept   { scramRequest = true; }

    struct Display
    {
        std::atomic<int> step { -1 };          // last played step, -1 = none
        std::atomic<bool> running { false };
        std::atomic<float> bpm { 120.0f };
        std::atomic<float> peak { 0.0f };      // max since the panel last read it
        std::atomic<bool> hostLocked { false }, lfoHigh { false };
        std::atomic<uint32_t> midiNotes { 0 };
    };

    Display& getDisplay() noexcept { return display; }
    double getCpuLoad() const { return loadMeasurer.getLoadAsProportion(); }

private:
    ps8::dsp::Settings readSettings (double bpm) const noexcept;
    void playStep (int step, float velocityScale) noexcept;
    double lfoBeats() const noexcept;
    void handleMidi (const juce::MidiMessage&) noexcept;

    juce::AudioProcessorValueTreeState state;

    struct Refs
    {
        std::atomic<float>* vco1Freq, * vco1Eg, * vco1Square, * vco1Level,
                          * vco2Freq, * vco2Eg, * vco2Square, * vco2Level,
                          * fmAmount, * fmOn, * hardSync,
                          * noiseLevel, * noisePink, * noiseVcf,
                          * cutoff, * resonance, * vcfEg, * highPass,
                          * pitchDecay, * vcfDecay, * vcaDecay, * vcaSlow, * drive, * volume,
                          * tempo, * run, * hostClock, * rate, * pitchTarget, * length, * scale, * glide,
                          * lfoRate, * lfoDivision, * lfoSync, * lfoShape, * lfoDepth, * lfoTarget;
        std::array<std::atomic<float>*, ps8::seq::maxSteps> pitch {}, velocity {}, on {};
    } refs {};

    ps8::dsp::Engine engine;
    ps8::seq::Sequencer sequencer;
    bool hostWasRunning = false;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> zeroBuffer, oversampledBuffer;
    int oversamplingFactor = 1;
    int maxChunk = 512;

    std::atomic<bool> resetRequest { false }, advanceRequest { false }, triggerRequest { false }, scramRequest { false };
    Display display;
    juce::AudioProcessLoadMeasurer loadMeasurer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PS8AudioProcessor)
};
