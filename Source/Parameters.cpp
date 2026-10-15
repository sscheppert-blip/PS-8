#include "Parameters.h"

#include "dsp/Engine.h"

namespace ps8::params
{
    juce::String id::stepPitch (int step)    { return "step" + juce::String (step + 1) + "Pitch"; }
    juce::String id::stepVelocity (int step) { return "step" + juce::String (step + 1) + "Velocity"; }
    juce::String id::stepOn (int step)       { return "step" + juce::String (step + 1) + "On"; }
    juce::String id::stepRatchet (int step)  { return "step" + juce::String (step + 1) + "Ratchet"; }
    juce::String id::stepChance (int step)   { return "step" + juce::String (step + 1) + "Chance"; }

    namespace
    {
        using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
        using ToText = std::function<juce::String (float, int)>;
        using FromText = std::function<float (const juce::String&)>;

        // Value -> text -> value -> text must give the same text (hosts and clap-validator check it), so
        // the unit and decimal switches look at the rounded value, and every text has a way back.
        juce::String hz (float f)
        {
            return f >= 999.5f ? juce::String (f / 1000.0f, 2) + " kHz" : juce::String (f, f < 99.95f ? 1 : 0) + " Hz";
        }

        float hzFromText (const juce::String& text)
        {
            const auto t = text.trim().toLowerCase();
            return t.getFloatValue() * (t.endsWith ("khz") ? 1000.0f : 1.0f);
        }

        juce::String seconds (float t)
        {
            return t >= 0.9995f ? juce::String (t, 2) + " s" : juce::String (t * 1000.0f, t < 0.09995f ? 1 : 0) + " ms";
        }

        float secondsFromText (const juce::String& text)   // a bare number counts as ms
        {
            const auto t = text.trim().toLowerCase();
            return t.getFloatValue() * (t.endsWith ("s") && ! t.endsWith ("ms") ? 1.0f : 0.001f);
        }

        juce::String percent (float v)   { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; }

        juce::String bipolar (float v)
        {
            const auto r = juce::roundToInt (v * 100.0f);
            return (r > 0 ? "+" : "") + juce::String (r) + " %";
        }

        float percentFromText (const juce::String& text) { return text.trim().getFloatValue() * 0.01f; }

        // Inverse of a mapping x = a * b^v
        float inverseExp (float x, float a, float b) { return std::log (std::max (x, 1.0e-6f) / a) / std::log (b); }

        void add (Layout& layout, const juce::String& paramId, const juce::String& name, float min, float max,
                  float defaultValue, ToText toText, FromText fromText, float interval = 0.0f)
        {
            auto attributes = juce::AudioParameterFloatAttributes().withStringFromValueFunction (std::move (toText));
            if (fromText)
                attributes = attributes.withValueFromStringFunction (std::move (fromText));

            layout.add (std::make_unique<juce::AudioParameterFloat> (
                juce::ParameterID { paramId, 1 }, name, juce::NormalisableRange<float> (min, max, interval), defaultValue,
                attributes));
        }

        void addUnit (Layout& layout, const char* paramId, const char* name, float defaultValue)
        {
            add (layout, paramId, name, 0.0f, 1.0f, defaultValue, [] (float v, int) { return percent (v); }, percentFromText);
        }

        void addBipolar (Layout& layout, const juce::String& paramId, const juce::String& name, float defaultValue)
        {
            add (layout, paramId, name, -1.0f, 1.0f, defaultValue, [] (float v, int) { return bipolar (v); }, percentFromText);
        }

        juce::AudioParameterBoolAttributes boolAttributes (const char* offText, const char* onText)
        {
            return juce::AudioParameterBoolAttributes()
                .withStringFromValueFunction ([offText, onText] (bool v, int) { return juce::String (v ? onText : offText); })
                .withValueFromStringFunction ([offText, onText] (const juce::String& text)
                {
                    const auto t = text.trim();
                    if (t.equalsIgnoreCase (onText))
                        return true;
                    if (t.equalsIgnoreCase (offText))
                        return false;
                    return juce::StringArray { "on", "yes", "true", "1" }.contains (t, true);
                });
        }

        void addBool (Layout& layout, const juce::String& paramId, const juce::String& name, bool defaultValue,
                      const char* offText, const char* onText)
        {
            layout.add (std::make_unique<juce::AudioParameterBool> (
                juce::ParameterID { paramId, 1 }, name, defaultValue, boolAttributes (offText, onText)));
        }
    }

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout (bool standalone)
    {
        Layout layout;
        const dsp::Settings d;

        auto vcoText = [] (float v, int) { return hz (dsp::vcoHz (v)); };
        auto vcoFromText = [] (const juce::String& t) { return inverseExp (hzFromText (t), 8.0f, 1024.0f); };
        auto decayText = [] (float v, int) { return seconds (dsp::decaySeconds (v)); };
        auto decayFromText = [] (const juce::String& t) { return inverseExp (secondsFromText (t), 0.003f, 10.0f / 0.003f); };

        add (layout, id::vco1Freq, "VCO 1 Frequency", 0.0f, 1.0f, d.vco1Freq, vcoText, vcoFromText);
        addBipolar (layout, id::vco1Eg, "VCO 1 EG Amount", d.vco1Eg);
        addBool (layout, id::vco1Square, "VCO 1 Wave", d.vco1Square, "Triangle", "Square");
        addUnit (layout, id::vco1Level, "VCO 1 Level", d.vco1Level);

        add (layout, id::vco2Freq, "VCO 2 Frequency", 0.0f, 1.0f, d.vco2Freq, vcoText, vcoFromText);
        addBipolar (layout, id::vco2Eg, "VCO 2 EG Amount", d.vco2Eg);
        addBool (layout, id::vco2Square, "VCO 2 Wave", d.vco2Square, "Triangle", "Square");
        addUnit (layout, id::vco2Level, "VCO 2 Level", d.vco2Level);

        addUnit (layout, id::fmAmount, "FM 1-2 Amount", d.fmAmount);
        addBool (layout, id::fmOn, "FM", d.fmOn, "Off", "On");
        addBool (layout, id::hardSync, "Hard Sync", d.hardSync, "Off", "On");

        addUnit (layout, id::noiseLevel, "Noise Level", d.noiseLevel);
        addBool (layout, id::noisePink, "Noise Color", d.noisePink, "White", "Pink");
        addUnit (layout, id::noiseVcf, "Noise VCF Mod", d.noiseVcf);

        add (layout, id::cutoff, "Cutoff", 0.0f, 1.0f, d.cutoff, [] (float v, int) { return hz (dsp::cutoffHz (v)); },
             [] (const juce::String& t) { return inverseExp (hzFromText (t), 20.0f, 1024.0f); });
        addUnit (layout, id::resonance, "Resonance", d.resonance);
        addBipolar (layout, id::vcfEg, "VCF EG Amount", d.vcfEg);
        addBool (layout, id::highPass, "Filter Mode", d.highPass, "Low Pass", "High Pass");

        add (layout, id::pitchDecay, "Pitch Decay", 0.0f, 1.0f, d.pitchDecay, decayText, decayFromText);
        add (layout, id::vcfDecay, "VCF Decay", 0.0f, 1.0f, d.vcfDecay, decayText, decayFromText);
        add (layout, id::vcaDecay, "VCA Decay", 0.0f, 1.0f, d.vcaDecay, decayText, decayFromText);
        addBool (layout, id::vcaSlow, "VCA Attack", d.vcaSlowAttack, "Fast", "Slow");
        addUnit (layout, id::drive, "Drive", d.drive);
        addUnit (layout, id::volume, "Volume", d.volume);

        add (layout, id::tempo, "Tempo", 20.0f, 300.0f, 120.0f,
             [] (float v, int) { return juce::String (v, 1) + " BPM"; }, nullptr, 0.1f);
        addBool (layout, id::run, "Run", false, "Stop", "Run");
        addBool (layout, id::hostClock, "Clock", ! standalone, "Internal", "Host");
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
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::root, 1 }, "Seq Root",
            juce::StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
        add (layout, id::swing, "Swing", 0.5f, 0.75f, 0.5f, [] (float v, int) { return percent (v); }, percentFromText);
        add (layout, id::glide, "Glide", 0.0f, 1.0f, 0.0f,
             [] (float v, int) { return v <= 0.0316f ? juce::String ("Off") : seconds (dsp::glideSeconds (v)); },
             [] (const juce::String& t) { return t.trim().equalsIgnoreCase ("Off") ? 0.0f : std::sqrt (std::max (0.0f, secondsFromText (t))); });
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id::viewPage, 1 }, "Step View", false,
            boolAttributes ("Steps 1-8", "Steps 9-16").withAutomatable (false)));
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { id::serviceOpen, 1 }, "Service Hatch", false,
            boolAttributes ("Closed", "Open").withAutomatable (false)));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::stepRow, 1 }, "Step Knob Row",
            juce::StringArray { "Velocity", "Ratchet", "Chance" }, 0,
            juce::AudioParameterChoiceAttributes().withAutomatable (false)));

        add (layout, id::lfoRate, "LFO Rate", 0.0f, 1.0f, 0.45f,
             [] (float v, int) { return juce::String (dsp::lfoHz (v), 2) + " Hz"; },
             [] (const juce::String& t) { return inverseExp (hzFromText (t), 0.05f, 800.0f); });
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::lfoDivision, 1 }, "LFO Sync Rate",
            juce::StringArray { "4 Bars", "2 Bars", "1 Bar", "1/2", "1/4", "1/8", "1/16", "1/32" }, 2));
        addBool (layout, id::lfoSync, "LFO Sync", false, "Free", "Sync");
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::lfoShape, 1 }, "LFO Shape",
            juce::StringArray { "Triangle", "Square", "Saw", "Ramp", "Sample & Hold" }, 0));
        addUnit (layout, id::lfoDepth, "LFO Depth", 0.0f);
        addUnit (layout, id::analog, "Analog", 0.0f);
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { id::lfoTarget, 1 }, "LFO Target",
            juce::StringArray { "Pitch", "VCO 2", "Cutoff", "FM", "Volume" }, 2));

        for (int i = 0; i < seq::maxSteps; ++i)
        {
            const auto n = juce::String (i + 1);
            add (layout, id::stepPitch (i), "Step " + n + " Pitch", -1.0f, 1.0f, 0.0f,
                 [] (float v, int)
                 {
                     const auto st = std::round (dsp::stepSemitones (v) * 10.0f) / 10.0f + 0.0f;   // + 0: no "-0.0"
                     return (st > 0.0f ? "+" : "") + juce::String (st, 1) + " st";
                 },
                 [] (const juce::String& t) { return t.trim().getFloatValue() / 24.0f; });
            add (layout, id::stepVelocity (i), "Step " + n + " Velocity", 0.0f, 1.0f, 1.0f,
                 [] (float v, int) { return percent (v); }, percentFromText);
            addBool (layout, id::stepOn (i), "Step " + n + " On", true, "Off", "On");
            layout.add (std::make_unique<juce::AudioParameterInt> (
                juce::ParameterID { id::stepRatchet (i), 1 }, "Step " + n + " Ratchet", 1, maxRatchet, 1,
                juce::AudioParameterIntAttributes().withStringFromValueFunction ([] (int v, int) { return juce::String (v) + "x"; })));
            add (layout, id::stepChance (i), "Step " + n + " Chance", 0.0f, 1.0f, 1.0f,
                 [] (float v, int) { return percent (v); }, percentFromText);
        }

        return layout;
    }
}
