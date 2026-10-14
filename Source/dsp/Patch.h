#pragma once

#include <array>
#include <cstdint>

// Patchbay: 9 outputs, 17 inputs. A patch is one bit mask of outputs per input; several cables into
// one input are summed. Signals use 1.0 = 5 V (pitch inputs: 1 V/oct, so 1.0 = 5 octaves). JUCE-free.
namespace ps8::patch
{
    enum Output : int
    {
        trigOut, velocityOut, pitchOut, vco1Out, vco2Out, vcoEgOut, vcfEgOut, vcaOut, vcaEgOut,
        numOutputs
    };

    enum Input : int
    {
        trigIn, vcaCvIn, velocityIn, vcaDecayIn,
        extIn, vcfDecayIn, noiseLevelIn, vcoDecayIn,
        vcfModIn, vco1CvIn, fmAmountIn, vco2CvIn,
        tempoIn, runIn, advClockIn, resetIn,
        vcoResetIn,   // extra: not on the original
        numInputs
    };

    using Mask = uint16_t;
    using Masks = std::array<Mask, numInputs>;

    // Stable names for the saved state
    inline constexpr const char* outputIds[numOutputs] {
        "trig", "velocity", "pitch", "vco1", "vco2", "vcoEg", "vcfEg", "vca", "vcaEg"
    };

    inline constexpr const char* inputIds[numInputs] {
        "trig", "vcaCv", "velocity", "vcaDecay", "ext", "vcfDecay", "noiseLevel", "vcoDecay",
        "vcfMod", "vco1Cv", "fmAmount", "vco2Cv", "tempo", "run", "advClock", "reset", "vcoReset"
    };

    // Gate inputs: high above 0.5 (2.5 V), low again below 0.25
    struct EdgeDetector
    {
        bool high = false;

        // true on a rising edge
        bool process (float v) noexcept
        {
            if (high)
            {
                high = v > 0.25f;
                return false;
            }

            high = v > 0.5f;
            return high;
        }
    };
}
