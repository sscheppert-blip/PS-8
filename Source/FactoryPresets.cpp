#include "FactoryPresets.h"
#include "Parameters.h"

namespace ps8
{
    namespace
    {
        namespace id = params::id;
        using Values = std::vector<std::pair<juce::String, float>>;

        // knob position as a clock hour: 7 = fully left, 12 = centre, 5 = fully right
        float clock (float hour)
        {
            return juce::jlimit (0.0f, 1.0f, (hour < 7.0f ? hour + 12.0f - 7.0f : hour - 7.0f) / 10.0f);
        }

        // bipolar knobs (EG AMOUNT) run -1 … +1
        float bipolar (float hour) { return 2.0f * clock (hour) - 1.0f; }

        // step pitches in semitones
        void pitches (Values& v, std::initializer_list<float> semitones)
        {
            int i = 0;
            for (const auto st : semitones)
                v.emplace_back (id::stepPitch (i++), st / 24.0f);
        }

        void velocities (Values& v, std::initializer_list<float> levels)
        {
            int i = 0;
            for (const auto level : levels)
                v.emplace_back (id::stepVelocity (i++), level);
        }

        Values kick()
        {
            return { { id::vco1Freq, clock (9) }, { id::vco1Eg, bipolar (2) }, { id::vco1Level, 1.0f },
                     { id::vco2Level, 0.0f }, { id::noiseLevel, 0.0f },
                     { id::cutoff, clock (1) }, { id::resonance, clock (9) }, { id::vcfEg, 0.0f },
                     { id::pitchDecay, clock (9) }, { id::vcaDecay, clock (12) } };
        }

        Values hats (float vcaDecay)
        {
            return { { id::vco1Square, 1.0f }, { id::vco2Square, 1.0f }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                     { id::vco1Freq, clock (3) }, { id::vco2Freq, clock (3.3f) }, { id::vco2Level, 0.85f },
                     { id::fmAmount, clock (2) }, { id::noiseLevel, clock (12) },
                     { id::highPass, 1.0f }, { id::cutoff, clock (2) }, { id::vcfEg, 0.0f },
                     { id::vcaDecay, vcaDecay } };
        }

        Values acid()
        {
            Values v { { id::vco1Square, 1.0f }, { id::vco1Freq, clock (10) }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                       { id::vco2Level, 0.0f }, { id::noiseLevel, 0.0f },
                       { id::cutoff, clock (9) }, { id::resonance, clock (3) }, { id::vcfEg, bipolar (2) },
                       { id::vcfDecay, clock (10) }, { id::vcaDecay, clock (1) },
                       { id::scale, 3.0f }, { id::glide, clock (9) }, { id::length, 16.0f } };
            pitches (v, { 0, 0, 12, 0, 3, 0, 7, 10, 0, 12, 0, 5, 3, 0, 15, 7 });
            velocities (v, { 1, 0.6f, 0.9f, 0.6f, 1, 0.6f, 0.8f, 0.7f, 1, 0.6f, 0.9f, 0.6f, 1, 0.7f, 0.9f, 0.6f });
            return v;
        }

        std::vector<FactoryPreset> build()
        {
            std::vector<FactoryPreset> all;

            // TRIG -> VCO RST: every hit starts the oscillators at the same point, so all kicks sound alike
            all.push_back ({ "Deep Kick", kick(), "trig>vcoReset" });

            {
                auto v = kick();
                v.push_back ({ id::vco1Freq, clock (11) });
                v.push_back ({ id::vco1Eg, bipolar (1) });
                v.push_back ({ id::scale, 3.0f });
                pitches (v, { 0, 3, 7, -5, 10, 0, 12, 5 });
                all.push_back ({ "Tom Melody", v, "trig>vcoReset" });
            }

            all.push_back ({ "Snare", { { id::vco1Freq, clock (11) }, { id::vco1Eg, bipolar (1) }, { id::vco1Level, 0.5f },
                                        { id::noiseLevel, clock (2) }, { id::highPass, 1.0f }, { id::cutoff, clock (11) },
                                        { id::pitchDecay, clock (8) }, { id::vcaDecay, clock (10) } }, {} });

            all.push_back ({ "Closed Hat", hats (clock (8)), {} });
            all.push_back ({ "Open Hat", hats (clock (11)), {} });

            all.push_back ({ "Laser Zap", { { id::vco1Freq, clock (1) }, { id::vco1Eg, 1.0f },
                                            { id::pitchDecay, clock (10) }, { id::vcaDecay, clock (10) },
                                            { id::resonance, clock (2) } }, {} });

            all.push_back ({ "Acid Line", acid(), {} });

            all.push_back ({ "Moving Groove", { { id::lfoTarget, 2.0f }, { id::lfoShape, 0.0f }, { id::lfoSync, 1.0f },
                                                { id::lfoDivision, 2.0f }, { id::lfoDepth, 0.5f } }, {} });

            all.push_back ({ "Random Pitches", { { id::lfoTarget, 0.0f }, { id::lfoShape, 4.0f }, { id::lfoSync, 1.0f },
                                                 { id::lfoDivision, 6.0f }, { id::lfoDepth, 0.2f } }, {} });

            // patchbay sounds
            {
                auto v = kick();
                v.push_back ({ id::vcaDecay, clock (11) });
                v.push_back ({ id::tempo, 90.0f });
                v.push_back ({ id::rate, 1.0f });
                all.push_back ({ "Rolls (INT)", v, "vcaEg>tempo" });
            }

            all.push_back ({ "Growl", { { id::vco2Freq, clock (1) }, { id::noiseVcf, clock (1) }, { id::resonance, clock (1) },
                                        { id::cutoff, clock (11) }, { id::vcaDecay, clock (11) } }, "vco2>vcfMod" });

            {
                Values v { { id::vco1Freq, clock (9.5f) }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                           { id::scale, 3.0f }, { id::glide, clock (10) },
                           { id::lfoTarget, 2.0f }, { id::lfoSync, 1.0f }, { id::lfoDivision, 1.0f }, { id::lfoDepth, 0.45f } };
                pitches (v, { 0, 7, 3, 10, 0, 5, 12, 7 });
                velocities (v, { 0.9f, 0.5f, 0.7f, 0.5f, 0.9f, 0.6f, 0.8f, 0.5f });
                all.push_back ({ "Drone", v, "velocity>vcaCv" });
            }

            all.push_back ({ "Feedback", { { id::noiseLevel, 0.4f }, { id::drive, 0.4f }, { id::resonance, clock (1) },
                                           { id::vcaDecay, clock (11) } }, "vca>ext" });

            {
                auto v = acid();
                v.push_back ({ id::noiseVcf, clock (2) });
                v.push_back ({ id::vcfEg, bipolar (1) });
                all.push_back ({ "Melodic Filter", v, "pitch>vcfMod" });
            }

            return all;
        }
    }

    const std::vector<FactoryPreset>& factoryPresets()
    {
        static const auto all = build();
        return all;
    }

    juce::ValueTree factoryTree (const FactoryPreset& preset)
    {
        juce::ValueTree tree ("PS8Preset");
        tree.setProperty ("version", 1, nullptr);
        tree.setProperty ("name", preset.name, nullptr);
        tree.setProperty ("patch", preset.patch, nullptr);

        // later entries win (variations override their base)
        for (const auto& [paramId, value] : preset.values)
        {
            auto child = tree.getChildWithProperty ("id", paramId);

            if (! child.isValid())
            {
                child = juce::ValueTree ("PARAM");
                child.setProperty ("id", paramId, nullptr);
                tree.appendChild (child, nullptr);
            }

            child.setProperty ("value", value, nullptr);
        }

        return tree;
    }
}
