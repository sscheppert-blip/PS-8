#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "dsp/Sequencer.h"

namespace ps8::params
{
    namespace id
    {
        inline constexpr const char* vco1Freq   = "vco1Freq";
        inline constexpr const char* vco1Eg     = "vco1Eg";
        inline constexpr const char* vco1Square = "vco1Square";
        inline constexpr const char* vco1Level  = "vco1Level";
        inline constexpr const char* vco2Freq   = "vco2Freq";
        inline constexpr const char* vco2Eg     = "vco2Eg";
        inline constexpr const char* vco2Square = "vco2Square";
        inline constexpr const char* vco2Level  = "vco2Level";
        inline constexpr const char* fmAmount   = "fmAmount";
        inline constexpr const char* fmOn       = "fmOn";
        inline constexpr const char* hardSync   = "hardSync";
        inline constexpr const char* noiseLevel = "noiseLevel";
        inline constexpr const char* noisePink  = "noisePink";
        inline constexpr const char* noiseVcf   = "noiseVcf";
        inline constexpr const char* cutoff     = "cutoff";
        inline constexpr const char* resonance  = "resonance";
        inline constexpr const char* vcfEg      = "vcfEg";
        inline constexpr const char* highPass   = "highPass";
        inline constexpr const char* pitchDecay = "pitchDecay";
        inline constexpr const char* vcfDecay   = "vcfDecay";
        inline constexpr const char* vcaDecay   = "vcaDecay";
        inline constexpr const char* vcaSlow    = "vcaSlowAttack";
        inline constexpr const char* drive      = "drive";
        inline constexpr const char* volume     = "volume";

        inline constexpr const char* tempo       = "tempo";
        inline constexpr const char* run         = "run";
        inline constexpr const char* hostClock   = "hostClock";
        inline constexpr const char* rate        = "rate";
        inline constexpr const char* pitchTarget = "seqPitchTarget";
        inline constexpr const char* length      = "seqLength";
        inline constexpr const char* scale       = "seqScale";
        inline constexpr const char* glide       = "glide";
        inline constexpr const char* viewPage    = "viewPage";
        inline constexpr const char* serviceOpen = "serviceOpen";

        inline constexpr const char* lfoRate     = "lfoRate";
        inline constexpr const char* lfoDivision = "lfoDivision";
        inline constexpr const char* lfoSync     = "lfoSync";
        inline constexpr const char* lfoShape    = "lfoShape";
        inline constexpr const char* lfoDepth    = "lfoDepth";
        inline constexpr const char* lfoTarget   = "lfoTarget";

        // per step: "step1Pitch", "step1Velocity", "step1On", …
        juce::String stepPitch (int step);
        juce::String stepVelocity (int step);
        juce::String stepOn (int step);
    }

    // LFO cycle length in quarter notes for each lfoDivision choice
    inline constexpr double lfoDivisionBeats[] { 16.0, 8.0, 4.0, 2.0, 1.0, 0.5, 0.25, 0.125 };

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
}
