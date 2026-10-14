#include "Sequencer.h"

#include <algorithm>
#include <cmath>

namespace ps8::seq
{
    double stepsPerQuarter (Rate rate) noexcept
    {
        switch (rate)
        {
            case Rate::quarter:          return 1.0;
            case Rate::eighth:           return 2.0;
            case Rate::eighthTriplet:    return 3.0;
            case Rate::sixteenth:        return 4.0;
            case Rate::sixteenthTriplet: return 6.0;
            case Rate::thirtySecond:     return 8.0;
        }

        return 4.0;
    }

    float quantise (float semitones, Scale scale) noexcept
    {
        static constexpr int major[] { 0, 2, 4, 5, 7, 9, 11 };
        static constexpr int minor[] { 0, 2, 3, 5, 7, 8, 10 };
        static constexpr int pentatonic[] { 0, 3, 5, 7, 10 };

        auto nearestIn = [semitones] (const int* degrees, int count)
        {
            const auto octave = std::floor (semitones / 12.0f) * 12.0f;
            auto best = octave;

            for (int o = -1; o <= 1; ++o)
                for (int i = 0; i < count; ++i)
                {
                    const auto candidate = octave + 12.0f * (float) o + (float) degrees[i];
                    if (std::abs (candidate - semitones) < std::abs (best - semitones))
                        best = candidate;
                }

            return best;
        };

        switch (scale)
        {
            case Scale::chromatic:  return std::round (semitones);
            case Scale::major:      return nearestIn (major, 7);
            case Scale::minor:      return nearestIn (minor, 7);
            case Scale::pentatonic: return nearestIn (pentatonic, 5);
            case Scale::off:        break;
        }

        return semitones;
    }

    void Sequencer::prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        running = false;
        lastHostEnd = -1.0;
    }

    void Sequencer::reset() noexcept
    {
        currentStep = -1;
        hostOffset = lastHostTick + 1;
    }

    void Sequencer::setLength (int numSteps) noexcept
    {
        length = std::clamp (numSteps, 1, maxSteps);
    }

    int Sequencer::advance() noexcept
    {
        currentStep = currentStep + 1 >= length ? 0 : currentStep + 1;
        return currentStep;
    }

    void Sequencer::setCurrent (int step) noexcept
    {
        currentStep = std::clamp (step, 0, maxSteps - 1);
    }

    int Sequencer::runInternal (int numSamples, double bpm, Rate rate, Tick* out, int maxTicks) noexcept
    {
        const auto samplesPerTick = sampleRate * 60.0 / (std::max (1.0, bpm) * stepsPerQuarter (rate));

        if (! running)
        {
            running = true;
            samplesToNext = 0.0;
        }

        int count = 0;

        while (samplesToNext < (double) numSamples)
        {
            if (count < maxTicks)
                out[count++] = { std::clamp ((int) samplesToNext, 0, numSamples - 1), advance() };

            samplesToNext += samplesPerTick;
        }

        samplesToNext -= (double) numSamples;
        return count;
    }

    int Sequencer::runHost (double ppqStart, double bpm, int numSamples, Rate rate, bool restarted,
                            Tick* out, int maxTicks) noexcept
    {
        const auto spq = stepsPerQuarter (rate);
        const auto ticksPerSample = std::max (1.0, bpm) / 60.0 * spq / sampleRate;
        const auto startTick = ppqStart * spq;
        const auto endTick = startTick + numSamples * ticksPerSample;

        // a jump (loop, locate) or a fresh start: continue from the new position
        if (restarted || std::abs (startTick - lastHostEnd) > 0.5)
        {
            lastHostTick = (int64_t) std::ceil (startTick - 1.0e-6) - 1;

            if (restarted)
                hostOffset = 0;
        }

        lastHostEnd = endTick;
        running = false;
        int count = 0;

        for (auto n = lastHostTick + 1; (double) n < endTick - 1.0e-9; ++n)
        {
            const auto position = std::clamp ((int) std::lround (((double) n - startTick) / ticksPerSample), 0, numSamples - 1);
            const auto step = (int) (((n - hostOffset) % length + length) % length);
            currentStep = step;
            lastHostTick = n;

            if (count < maxTicks)
                out[count++] = { position, step };
        }

        return count;
    }
}
