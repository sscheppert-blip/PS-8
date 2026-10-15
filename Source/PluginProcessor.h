#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <clap-juce-extensions/clap-juce-extensions.h>

#include "Patterns.h"
#include "Presets.h"
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

    //==============================================================================
    // Patchbay (message thread). Not automatable; saved with the plugin state.
    bool isConnected (ps8::patch::Output, ps8::patch::Input) const noexcept;
    void connect (ps8::patch::Output, ps8::patch::Input) noexcept;
    void disconnect (ps8::patch::Output, ps8::patch::Input) noexcept;
    void clearPatch() noexcept;
    ps8::patch::Masks getPatch() const noexcept;
    void setPatch (const ps8::patch::Masks&) noexcept;
    // changes whenever the patch changes (also when a saved state is loaded)
    uint32_t getPatchVersion() const noexcept { return patchVersion.load(); }

    // Presets; the plugin state also remembers the preset's name
    ps8::Presets& getPresets() noexcept { return *presets; }
    ps8::Patterns& getPatterns() noexcept { return *patterns; }
    // changes whenever a saved state is loaded
    uint32_t getStateVersion() const noexcept { return stateVersion.load(); }

    static juce::String patchToString (const ps8::patch::Masks&);
    static ps8::patch::Masks patchFromString (const juce::String&);
    double getCpuLoad() const { return loadMeasurer.getLoadAsProportion(); }

private:
    ps8::dsp::Settings readSettings (double bpm) const noexcept;
    // clocked = played by the sequencer clock (internal, host or ADV/CLK cable): only then does the
    // step's chance apply. Ratchets always play.
    void playStep (int step, float velocityScale, bool clocked = false) noexcept;
    void playRatchets() noexcept;
    // a clocked step from the grid: offbeats wait for the swing
    void clockStep (int step, bool offbeat) noexcept;
    double lfoBeats() const noexcept;
    void handleMidi (const juce::MidiMessage&) noexcept;
    bool readExtInput (juce::AudioBuffer<float>&, int numSamples);

    static constexpr const char* patchProperty = "patch";
    static constexpr const char* presetProperty = "preset";

    juce::AudioProcessorValueTreeState state;

    struct Refs
    {
        std::atomic<float>* vco1Freq, * vco1Eg, * vco1Square, * vco1Level,
                          * vco2Freq, * vco2Eg, * vco2Square, * vco2Level,
                          * fmAmount, * fmOn, * hardSync,
                          * noiseLevel, * noisePink, * noiseVcf,
                          * cutoff, * resonance, * vcfEg, * highPass,
                          * pitchDecay, * vcfDecay, * vcaDecay, * vcaSlow, * drive, * volume,
                          * tempo, * run, * hostClock, * rate, * pitchTarget, * length, * scale, * glide, * root, * swing,
                          * lfoRate, * lfoDivision, * lfoSync, * lfoShape, * lfoDepth, * lfoTarget;
        std::array<std::atomic<float>*, ps8::seq::maxSteps> pitch {}, velocity {}, on {}, ratchet {}, chance {};
    } refs {};

    ps8::dsp::Engine engine;
    ps8::seq::Sequencer sequencer;
    bool hostWasRunning = false;

    std::array<std::atomic<ps8::patch::Mask>, ps8::patch::numInputs> patchMasks {};
    std::atomic<uint32_t> patchVersion { 0 }, stateVersion { 0 };
    std::unique_ptr<ps8::Presets> presets;
    std::unique_ptr<ps8::Patterns> patterns;
    bool editorCreated = false;
    ps8::patch::EdgeDetector runGate, clockEdge, resetEdge;

    // Ratchets: the remaining hits of the current step, spread evenly over its length
    struct Ratchet
    {
        int remaining = 0;
        double interval = 0.0, countdown = 0.0;
        float velocity = 0.0f, semitones = 0.0f;
        bool clocked = false;
    } ratchet;

    // swing: the offbeat step waiting to be played
    struct Swing
    {
        int step = -1;
        double countdown = 0.0;
    } swingPending;

    double hostSampleRate = 48000.0;
    double stepSamples = 6000.0;        // length of the current step, for the ratchets
    double samplesSinceClock = -1.0;    // ADV/CLK cable: time since the last clock edge
    juce::Random random;

    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioBuffer<float> zeroBuffer, oversampledBuffer, extBuffer;
    int oversamplingFactor = 1;
    int maxChunk = 512;

    std::atomic<bool> resetRequest { false }, advanceRequest { false }, triggerRequest { false }, scramRequest { false };
    Display display;
    juce::AudioProcessLoadMeasurer loadMeasurer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PS8AudioProcessor)
};
