#pragma once

#include <cstdint>

// Step clock and counter (1–16 steps). Runs either on its own tempo (internal clock) or locked to the
// host's beat position, so steps line up with the bar. JUCE-free.
namespace ps8::seq
{
    constexpr int maxSteps = 16;

    enum class Rate { quarter, eighth, eighthTriplet, sixteenth, sixteenthTriplet, thirtySecond };
    double stepsPerQuarter (Rate) noexcept;

    // Pitch quantiser for the step pitch knobs
    enum class Scale { off, chromatic, major, minor, pentatonic };
    float quantise (float semitones, Scale) noexcept;

    struct Tick
    {
        int position = 0;   // sample in the block
        int step = 0;       // 0..length-1
    };

    class Sequencer
    {
    public:
        void prepare (double sampleRate) noexcept;

        // Internal clock: the first tick after a stop falls on the first sample.
        int runInternal (int numSamples, double bpm, Rate rate, Tick* out, int maxTicks) noexcept;

        // Internal clock, one sample at a time (for a tempo that changes every sample).
        // Returns the step played on this sample, or -1.
        int clockInternal (double bpm, Rate rate) noexcept;

        // Host clock: one tick per grid line between ppqStart and the end of the block.
        // Pass restarted = true when the transport has just started or jumped.
        int runHost (double ppqStart, double bpm, int numSamples, Rate rate, bool restarted,
                     Tick* out, int maxTicks) noexcept;

        void stop() noexcept { running = false; }

        void setLength (int numSteps) noexcept;
        int getLength() const noexcept { return length; }

        // The next tick plays step 1.
        void reset() noexcept;

        // Manual stepping (ADV button / MIDI)
        int advance() noexcept;
        void setCurrent (int step) noexcept;
        int current() const noexcept { return currentStep; }

    private:
        double sampleRate = 48000.0;
        bool running = false;
        double samplesToNext = 0.0;
        double stepsToNext = 0.0;   // clockInternal
        int currentStep = -1;
        int length = 8;

        int64_t lastHostTick = -1;
        int64_t hostOffset = 0;
        double lastHostEnd = -1.0;
    };
}
