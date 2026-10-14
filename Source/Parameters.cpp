#include "Parameters.h"

#include "dsp/Engine.h"

namespace ps8::params
{
    juce::String id::stepPitch (int step)    { return "step" + juce::String (step + 1) + "Pitch"; }
    juce::String id::stepVelocity (int step) { return "step" + juce::String (step + 1) + "Velocity"; }
    juce::String id::stepOn (int step)       { return "step" + juce::String (step + 1) + "On"; }

    namespace
    {
        using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
        using ToText = std::function<juce::String (float, int)>;

        juce::String hz (float f)
        {
            return f >= 1000.0f ? juce::String (f / 1000.0f, 2) + " kHz" : juce::String (f, f < 100.0f ? 1 : 0) + " Hz";
        }

        juce::String seconds (float t)
        {
            return t >= 1.0f ? juce::String (t, 2) + " s" : juce::String (t * 1000.0f, t < 0.1f ? 1 : 0) + " ms";
        }

        juce::String percent (float v)   { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }
        juce::String bipolar (float v)   { return (v > 0.0f ? "+" : "") + juce::String (juce::roundToInt (v * 100.0f)) + " %"; }

        void add (Layout& layout, const juce::String& paramId, const juce::String& name, float min, float max,
                  float defaultValue, ToText toText, float interval = 0.0f)
        {
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { paramId, 1 }, name, juce::NormalisableRange<float> (min, max, interval), defaultValue,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toText))));
        }

        void addUnit (Layout& layout, const char* paramId, const char* name, float defaultValue)
        {
            add (layout, paramId, name, 0.0f, 1.0f, defaultValue, [] (float v, int) { return percent (v); });
        }

        void addBipolar (Layout& layout, const juce::String& paramId, const juce::String& name, float defaultValue)
        {
            add (layout, paramId, name, -1.0f, 1.0f, defaultValue, [] (float v, int) { return bipolar (v); });
        }

        void addBool (Layout& layout, const juce::String& paramId, const juce::String& name, bool defaultValue,
                      const char* offText, const char* onText)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID { paramId, 1 }, name, defaultValue,
                juce::AudioParameterBoolAttributes().withStringFromValueFunction (
                    [offText, onText] (bool v, int) { return juce::String (v ? onText : offText); })));
        }
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
    {
        Layout layout;
        const dsp::Settings d;

        auto vcoText = [] (float v, int) { return hz (dsp::vcoHz (v)); };
        auto decayText = [] (float v, int) { return seconds (dsp::decaySeconds (v)); };

        add (layout, id::vco1Freq, "VCO 1 Frequency", 0.0f, 1.0f, d.vco1Freq, vcoText);
        addBipolar (layout, id::vco1Eg, "VCO 1 EG Amount", d.vco1Eg);
        addBool (layout, id::vco1Square, "VCO 1 Wave", d.vco1Square, "Triangle", "Square");
        addUnit (layout, id::vco1Level, "VCO 1 Level", d.vco1Level);

        add (layout, id::vco2Freq, "VCO 2 Frequency", 0.0f, 1.0f, d.vco2Freq, vcoText);
        addBipolar (layout, id::vco2Eg, "VCO 2 EG Amount", d.vco2Eg);
        addBool (layout, id::vco2Square, "VCO 2 Wave", d.vco2Square, "Triangle", "Square");
        addUnit (layout, id::vco2Level, "VCO 2 Level", d.vco2Level);

        addUnit (layout, id::fmAmount, "FM 1-2 Amount", d.fmAmount);
        addBool (layout, id::fmOn, "FM", d.fmOn, "Off", "On");
        addBool (layout, id::hardSync, "Hard Sync", d.hardSync, "Off", "On");

        addUnit (layout, id::noiseLevel, "Noise Level", d.noiseLevel);
        addBool (layout, id::noisePink, "Noise Color", d.noisePink, "White", "Pink");
        addUnit (layout, id::noiseVcf, "Noise VCF Mod", d.noiseVcf);

        add (layout, id::cutoff, "Cutoff", 0.0f, 1.0f, d.cutoff, [] (float v, int) { return hz (dsp::cutoffHz (v)); });
        addUnit (layout, id::resonance, "Resonance", d.resonance);
        addBipolar (layout, id::vcfEg, "VCF EG Amount", d.vcfEg);
        addBool (layout, id::highPass, "Filter Mode", d.highPass, "Low Pass", "High Pass");

        add (layout, id::pitchDecay, "Pitch Decay", 0.0f, 1.0f, d.pitchDecay, decayText);
        add (layout, id::vcfDecay, "VCF Decay", 0.0f, 1.0f, d.vcfDecay, decayText);
        add (layout, id::vcaDecay, "VCA Decay", 0.0f, 1.0f, d.vcaDecay, decayText);
        addBool (layout, id::vcaSlow, "VCA Attack", d.vcaSlowAttack, "Fast", "Slow");
        addUnit (layout, id::drive, "Drive", d.drive);
        addUnit (layout, id::volume, "Volume", d.volume);

        add (layout, id::tempo, "Tempo", 20.0f, 300.0f, 120.0f,
             [] (float v, int) { return juce::String (v, 1) + " BPM"; }, 0.1f);
        addBool (layout, id::run, "Run", false, "Stop", "Run");
        addBool (layout, id::hostClock, "Clock", true, "Internal", "Host");
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::rate, 1 }, "Rate",
            juce::StringArray { "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" }, (int) seq::Rate::sixteenth));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::pitchTarget, 1 }, "Seq Pitch To",
            juce::StringArray { "VCO 1+2", "Off", "VCO 2" }, 0));
        layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id::length, 1 }, "Seq Length", 1, seq::maxSteps, 8));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::scale, 1 }, "Seq Pitch Scale",
            juce::StringArray { "Off", "Chromatic", "Major", "Minor", "Pentatonic" }, 0));
        add (layout, id::glide, "Glide", 0.0f, 1.0f, 0.0f,
             [] (float v, int) { return v <= 0.0316f ? juce::String ("Off") : seconds (dsp::glideSeconds (v)); });
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id::viewPage, 1 }, "Step View", false,
            juce::AudioParameterBoolAttributes().withAutomatable (false).withStringFromValueFunction (
                [] (bool v, int) { return juce::String (v ? "Steps 9-16" : "Steps 1-8"); })));

        add (layout, id::lfoRate, "LFO Rate", 0.0f, 1.0f, 0.45f,
             [] (float v, int) { return juce::String (dsp::lfoHz (v), 2) + " Hz"; });
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::lfoDivision, 1 }, "LFO Sync Rate",
            juce::StringArray { "4 Bars", "2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2));
        addBool (layout, id::lfoSync, "LFO Sync", false, "Free", "Sync");
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::lfoShape, 1 }, "LFO Shape",
            juce::StringArray { "Triangle", "Square", "Saw", "Ramp", "Sample & Hold" }, 0));
        addUnit (layout, id::lfoDepth, "LFO Depth", 0.0f);
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::lfoTarget, 1 }, "LFO Target",
            juce::StringArray { "Pitch", "VCO 2", "Cutoff", "FM", "Volume" }, 2));

        for (int i = 0; i < seq::maxSteps; ++i)
        {
            const auto n = juce::String (i + 1);
            add (layout, id::stepPitch (i), "Step " + n + " Pitch", -1.0f, 1.0f, 0.0f,
                 [] (float v, int)
                 {
                     const auto st = dsp::stepSemitones (v);
                     return (st > 0.0f ? "+" : "") + juce::String (st, 1) + " st";
                 });
            add (layout, id::stepVelocity (i), "Step " + n + " Velocity", 0.0f, 1.0f, 1.0f,
                 [] (float v, int) { return percent (v); });
            addBool (layout, id::stepOn (i), "Step " + n + " On", true, "Off", "On");
        }

        return layout;
    }
}
