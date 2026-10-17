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

        // choice parameters, as plain values
        constexpr float quarter = 0, eighth = 1, sixteenthTriplet = 4, thirtySecond = 5;               // RATE
        constexpr float scaleMinor = 3, scalePentatonic = 4;                                          // SCALE
        constexpr float triangle = 0, square = 1, sampleHold = 4;                                     // LFO SHAPE
        constexpr float toPitch = 0, toCutoff = 2;                                                    // LFO TARGET
        constexpr float twoBars = 1, oneBar = 2, sixteenthNote = 6;                                   // LFO SYNC RATE

        // per-step rows, from step 1 on
        void row (Values& v, juce::String (*paramId) (int), std::initializer_list<float> values)
        {
            int i = 0;
            for (const auto value : values)
                v.emplace_back (paramId (i++), value);
        }

        void pitches (Values& v, std::initializer_list<float> semitones)
        {
            int i = 0;
            for (const auto st : semitones)
                v.emplace_back (id::stepPitch (i++), st / 24.0f);
        }

        void velocities (Values& v, std::initializer_list<float> levels) { row (v, id::stepVelocity, levels); }
        void stepsOn (Values& v, std::initializer_list<float> on)        { row (v, id::stepOn, on); }
        void ratchets (Values& v, std::initializer_list<float> hits)     { row (v, id::stepRatchet, hits); }
        void chances (Values& v, std::initializer_list<float> chance)    { row (v, id::stepChance, chance); }

        Values operator+ (Values a, const Values& b)
        {
            a.insert (a.end(), b.begin(), b.end());
            return a;
        }

        //==============================================================================
        // The kick, matched to recordings of the original: VCO 1 (triangle) at about 45 Hz sweeps down
        // from 3–4 octaves above within ~50 ms; the filter sits almost closed (~100 Hz), so the body is
        // nearly a sine; the click is the VCA's hard onset; the VCA holds the body for a while.
        Values kick()
        {
            return { { id::vco1Freq, 0.25f }, { id::vco1Eg, 0.88f }, { id::vco1Level, 1.0f }, { id::vco1Square, 0.0f },
                     { id::vco2Level, 0.0f }, { id::noiseLevel, 0.0f },
                     { id::cutoff, 0.26f }, { id::resonance, 0.21f }, { id::vcfEg, 0.27f }, { id::vcfDecay, 0.365f },
                     { id::pitchDecay, 0.51f }, { id::vcaDecay, 0.60f } };
        }

        Values tom()
        {
            return { { id::vco1Freq, clock (11) }, { id::vco1Eg, bipolar (1) }, { id::vco1Level, 1.0f },
                     { id::vco2Level, 0.0f }, { id::noiseLevel, 0.0f },
                     { id::cutoff, clock (1) }, { id::resonance, clock (9) }, { id::vcfEg, 0.0f },
                     { id::pitchDecay, clock (9) }, { id::vcaDecay, clock (11.5f) } };
        }

        Values snare()
        {
            return { { id::vco1Freq, clock (11) }, { id::vco1Eg, bipolar (1) }, { id::vco1Level, 0.5f },
                     { id::noiseLevel, clock (2) }, { id::highPass, 1.0f }, { id::cutoff, clock (11) },
                     { id::pitchDecay, clock (8) }, { id::vcaDecay, clock (10) } };
        }

        Values hats (float vcaDecay)
        {
            return { { id::vco1Square, 1.0f }, { id::vco2Square, 1.0f }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                     { id::vco1Freq, clock (3) }, { id::vco2Freq, clock (3.3f) }, { id::vco2Level, 0.85f },
                     { id::fmAmount, clock (2) }, { id::noiseLevel, clock (12) },
                     { id::highPass, 1.0f }, { id::cutoff, clock (2) }, { id::vcfEg, 0.0f },
                     { id::vcaDecay, vcaDecay } };
        }

        // closed hat matched to recordings of the original: noise through the resonant high-pass, a little
        // of VCO 1 underneath
        Values closedHat()
        {
            return { { id::vco1Freq, 0.38f }, { id::vco1Eg, 0.0f }, { id::vco1Level, 0.35f }, { id::vco2Level, 0.0f },
                     { id::vco2Eg, 0.0f }, { id::noiseLevel, 0.92f }, { id::highPass, 1.0f },
                     { id::cutoff, 0.70f }, { id::resonance, 0.46f }, { id::vcfEg, 0.25f },
                     { id::vcfDecay, 0.28f }, { id::vcaDecay, 0.36f } };
        }

        Values acid()
        {
            Values v { { id::vco1Square, 1.0f }, { id::vco1Freq, clock (10) }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                       { id::vco2Level, 0.0f }, { id::noiseLevel, 0.0f },
                       { id::cutoff, clock (9) }, { id::resonance, clock (3) }, { id::vcfEg, bipolar (2) },
                       { id::vcfDecay, clock (10) }, { id::vcaDecay, clock (1) },
                       { id::scale, scaleMinor }, { id::glide, clock (9) }, { id::length, 16.0f } };
            pitches (v, { 0, 0, 12, 0, 3, 0, 7, 10, 0, 12, 0, 5, 3, 0, 15, 7 });
            velocities (v, { 1, 0.6f, 0.9f, 0.6f, 1, 0.6f, 0.8f, 0.7f, 1, 0.6f, 0.9f, 0.6f, 1, 0.7f, 0.9f, 0.6f });
            return v;
        }

        Values subBass()
        {
            Values v { { id::vco1Freq, clock (10) }, { id::vco1Eg, 0.0f }, { id::vco2Freq, clock (10) }, { id::vco2Eg, 0.0f },
                       { id::vco2Level, 0.25f }, { id::noiseLevel, 0.0f },
                       { id::cutoff, clock (11) }, { id::resonance, clock (9) }, { id::vcfEg, bipolar (1) },
                       { id::vcfDecay, clock (12) }, { id::vcaDecay, clock (1) }, { id::vcaSlow, 1.0f },
                       { id::glide, clock (8.5f) }, { id::scale, scaleMinor } };
            pitches (v, { 0, 0, -5, 0, 3, 0, -2, -5 });
            return v;
        }

        //==============================================================================
        std::vector<FactoryPreset> build()
        {
            std::vector<FactoryPreset> all;
            juce::String category;

            auto add = [&] (const juce::String& name, Values values, const juce::String& patch = {})
            {
                all.push_back ({ name, std::move (values), patch, category });
            };

            // TRIG -> VCO RST: every hit starts the oscillators at the same point, so all kicks sound alike
            category = "KICK";
            add ("Deep Kick", kick(), "trig>vcoReset");
            add ("Punch Kick", kick() + Values { { id::vco1Eg, 0.70f }, { id::pitchDecay, 0.476f }, { id::vcaDecay, 0.536f },
                                                  { id::cutoff, 0.24f }, { id::drive, 0.35f } },
                 "trig>vcoReset");
            // soft: a slower onset (VCA ATTACK SLOW) takes the click away
            add ("Long Boom", kick() + Values { { id::vco1Freq, 0.235f }, { id::pitchDecay, 0.57f }, { id::vcaDecay, 0.69f },
                                                 { id::cutoff, 0.24f }, { id::vcaSlow, 1.0f } },
                 "trig>vcoReset");
            add ("Distorted Kick", kick() + Values { { id::vco1Freq, 0.27f }, { id::drive, 0.9f }, { id::resonance, 0.35f },
                                                      { id::cutoff, 0.30f }, { id::volume, 0.55f } },
                 "trig>vcoReset");
            // the oscillator runs free and ANALOG is up: every kick a little different, like a worn machine
            add ("Worn Kick", kick() + Values { { id::analog, 0.85f } });
            add ("Tight Kick", kick() + Values { { id::vco1Freq, 0.305f }, { id::vco1Eg, 0.76f }, { id::pitchDecay, 0.484f },
                                                  { id::cutoff, 0.24f } },
                 "trig>vcoReset");
            add ("Slow Sweep Kick", kick() + Values { { id::vco1Freq, 0.223f }, { id::vco1Eg, 0.745f }, { id::pitchDecay, 0.652f },
                                                       { id::vcaDecay, 0.617f }, { id::cutoff, 0.278f }, { id::vcfEg, 0.16f },
                                                       { id::vcfDecay, 0.443f } },
                 "trig>vcoReset");

            category = "TOM";
            {
                auto v = tom() + Values { { id::scale, scaleMinor } };
                pitches (v, { 0, 3, 7, -5, 10, 0, 12, 5 });
                add ("Tom Melody", v, "trig>vcoReset");
            }
            {
                auto v = tom() + Values { { id::vco1Freq, clock (10.5f) }, { id::vco2Freq, clock (11.2f) }, { id::vco2Level, 0.3f },
                                          { id::vco2Square, 0.0f }, { id::noiseLevel, 0.08f }, { id::scale, scaleMinor } };
                pitches (v, { 0, -5, -3, 0, -7, -5, 0, -12 });
                velocities (v, { 1, 0.7f, 0.8f, 0.6f, 1, 0.7f, 0.9f, 0.6f });
                add ("Low Toms", v, "trig>vcoReset");
            }
            {
                auto v = tom() + Values { { id::vco1Freq, clock (11.5f) }, { id::vco1Eg, bipolar (3.5f) },
                                          { id::pitchDecay, clock (11.5f) }, { id::vcaDecay, clock (12) },
                                          { id::noiseLevel, 0.15f }, { id::cutoff, clock (2) }, { id::resonance, clock (10) } };
                stepsOn (v, { 1, 0, 0, 1, 0, 0, 1, 1 });
                ratchets (v, { 1, 1, 1, 1, 1, 1, 1, 2 });
                add ("Disco Tom", v, "trig>vcoReset");
            }
            {
                auto v = tom() + Values { { id::rate, sixteenthTriplet }, { id::length, 6.0f }, { id::scale, scalePentatonic } };
                pitches (v, { 0, 3, 7, 10, 7, 3 });
                velocities (v, { 1, 0.5f, 0.7f, 0.9f, 0.5f, 0.6f });
                add ("Triplet Toms", v, "trig>vcoReset");
            }

            category = "SNARE / CLAP";
            add ("Snare", snare());
            add ("Tight Snare", snare() + Values { { id::vco1Freq, clock (11.2f) }, { id::vco1Eg, bipolar (12.8f) },
                                                    { id::vco1Level, 0.45f }, { id::vco2Freq, clock (12) }, { id::vco2Level, 0.3f },
                                                    { id::vco2Square, 0.0f }, { id::noiseLevel, 0.85f }, { id::cutoff, clock (11.5f) },
                                                    { id::pitchDecay, clock (8.5f) }, { id::vcaDecay, clock (9.5f) } });
            {
                auto v = Values { { id::vco1Level, 0.0f }, { id::vco2Level, 0.0f }, { id::noiseLevel, 1.0f },
                                  { id::cutoff, clock (1.2f) }, { id::resonance, clock (10.5f) }, { id::vcfEg, bipolar (1) },
                                  { id::vcfDecay, clock (9.5f) }, { id::vcaDecay, clock (10.5f) },
                                  { id::volume, 1.0f }, { id::drive, 0.45f } };
                stepsOn (v, { 0, 0, 1, 0, 0, 0, 1, 0 });
                ratchets (v, { 1, 1, 3, 1, 1, 1, 3, 1 });
                add ("Noise Clap", v);
            }
            add ("Rim Shot", { { id::vco1Freq, clock (1.2f) }, { id::vco1Eg, bipolar (12.5f) }, { id::vco1Level, 0.8f },
                               { id::vco2Freq, clock (2.2f) }, { id::vco2Level, 0.4f }, { id::noiseLevel, 0.2f },
                               { id::cutoff, clock (3) }, { id::resonance, clock (10) }, { id::drive, 0.3f },
                               { id::pitchDecay, clock (8) }, { id::vcaDecay, clock (8.8f) }, { id::volume, 0.65f } }, "trig>vcoReset");
            {
                auto v = snare() + Values { { id::vcaDecay, clock (9.5f) } };
                velocities (v, { 1, 0.2f, 0.3f, 0.2f, 1, 0.3f, 0.2f, 0.5f });
                add ("Ghost Notes", v);
            }

            category = "HAT / METAL";
            add ("Closed Hat", closedHat());
            add ("Open Hat", { { id::vco1Level, 0.0f }, { id::vco2Level, 0.0f }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                               { id::noiseLevel, 0.93f }, { id::highPass, 1.0f }, { id::cutoff, 0.72f },
                               { id::resonance, 0.5f }, { id::vcfEg, 0.2f }, { id::vcfDecay, 0.3f },
                               { id::vcaDecay, 0.6f }, { id::volume, 0.6f } });
            add ("Ride Metal", hats (clock (1)) + Values { { id::fmAmount, 0.8f }, { id::vco1Freq, clock (3.2f) },
                                                           { id::vco2Freq, clock (3.7f) }, { id::noiseLevel, 0.2f },
                                                           { id::volume, 0.55f } });
            {
                auto v = closedHat() + Values { { id::analog, 0.6f }, { id::swing, 0.62f } };
                velocities (v, { 1, 0.45f, 0.75f, 0.45f, 1, 0.45f, 0.75f, 0.5f });
                chances (v, { 1, 0.8f, 1, 0.6f, 1, 0.8f, 1, 0.7f });
                add ("Swing Hats", v);
            }
            add ("Bit Hats", closedHat() + Values { { id::vco2Square, 1.0f }, { id::vco2Freq, clock (3) }, { id::noiseVcf, 0.7f } },
                 "vco2>vcfMod");
            add ("Metal Gong", { { id::vco1Freq, clock (12.2f) }, { id::vco2Freq, clock (1.1f) }, { id::fmAmount, 0.9f },
                                 { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f }, { id::vco1Level, 0.8f }, { id::vco2Level, 0.6f },
                                 { id::noiseLevel, 0.0f }, { id::cutoff, clock (2) }, { id::resonance, clock (11) },
                                 { id::vcfEg, bipolar (1) }, { id::vcfDecay, clock (1) }, { id::vcaDecay, clock (2.2f) },
                                 { id::rate, quarter }, { id::length, 4.0f } });

            // metallic hits matched to recordings of the original: two squares, FM 1>2 and noise
            add ("Metal Hit", { { id::vco1Square, 1.0f }, { id::vco2Square, 1.0f }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                                { id::vco1Freq, 0.546f }, { id::vco2Freq, 0.854f }, { id::vco1Level, 0.41f },
                                { id::vco2Level, 0.36f }, { id::fmAmount, 0.38f }, { id::noiseLevel, 0.34f },
                                { id::cutoff, 0.79f }, { id::resonance, 0.53f }, { id::vcfEg, 0.75f },
                                { id::vcfDecay, 0.54f }, { id::vcaDecay, 0.35f } });
            add ("Clank", { { id::vco1Square, 1.0f }, { id::vco2Square, 1.0f }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                            { id::vco1Freq, 0.558f }, { id::vco2Freq, 0.849f }, { id::vco1Level, 0.81f },
                            { id::vco2Level, 0.22f }, { id::fmAmount, 0.39f }, { id::noiseLevel, 0.78f },
                            { id::cutoff, 0.75f }, { id::resonance, 0.0f }, { id::vcfEg, -0.25f },
                            { id::vcfDecay, 0.3f }, { id::vcaDecay, 0.48f } });

            category = "PERCUSSION";
            add ("Cowbell", { { id::vco1Square, 1.0f }, { id::vco2Square, 1.0f }, { id::vco1Freq, clock (1.8f) },
                              { id::vco2Freq, clock (2.3f) }, { id::vco1Level, 0.7f }, { id::vco2Level, 0.7f },
                              { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f }, { id::fmAmount, 0.0f }, { id::noiseLevel, 0.0f },
                              { id::cutoff, clock (2.5f) }, { id::resonance, clock (10.5f) }, { id::vcfEg, bipolar (1) },
                              { id::vcaDecay, clock (10.2f) } }, "trig>vcoReset");
            add ("Clave", { { id::vco1Freq, clock (2.6f) }, { id::vco1Eg, bipolar (12.5f) }, { id::vco2Level, 0.0f },
                            { id::noiseLevel, 0.0f }, { id::cutoff, clock (3.5f) }, { id::resonance, clock (12) },
                            { id::pitchDecay, clock (7.5f) }, { id::vcaDecay, clock (8.5f) } }, "trig>vcoReset");
            {
                auto v = Values { { id::vco1Level, 0.0f }, { id::vco2Level, 0.0f }, { id::noiseLevel, 0.9f },
                                  { id::highPass, 1.0f }, { id::cutoff, clock (2.5f) }, { id::vcaSlow, 1.0f },
                                  { id::vcaDecay, clock (9.2f) }, { id::swing, 0.6f }, { id::analog, 0.5f } };
                velocities (v, { 0.55f, 1, 0.5f, 0.9f, 0.55f, 1, 0.5f, 0.85f });
                chances (v, { 1, 1, 0.7f, 1, 1, 1, 0.6f, 1 });
                add ("Shaker", v);
            }
            {
                auto v = tom() + Values { { id::vco1Freq, clock (12.5f) }, { id::pitchDecay, clock (8.5f) },
                                          { id::vcaDecay, clock (9.5f) }, { id::swing, 0.66f }, { id::scale, scalePentatonic } };
                pitches (v, { 0, 7, 3, 7, 10, 7, 3, 12 });
                velocities (v, { 1, 0.6f, 0.8f, 0.5f, 1, 0.6f, 0.7f, 0.9f });
                chances (v, { 1, 0.8f, 1, 0.6f, 1, 0.8f, 0.9f, 1 });
                add ("Swing Perc", v, "trig>vcoReset");
            }

            {
                // FM percussion matched to a recording of the original: VCO 2 modulated by VCO 1, its own
                // pitch falling a little, the filter low and resonant
                auto v = Values { { id::vco1Square, 1.0f }, { id::vco2Square, 1.0f }, { id::vco1Freq, 0.618f },
                                  { id::vco1Eg, 0.0f }, { id::vco1Level, 0.14f }, { id::vco2Freq, 0.618f },
                                  { id::vco2Eg, -0.38f }, { id::vco2Level, 0.64f }, { id::fmAmount, 0.5f },
                                  { id::noiseLevel, 0.0f }, { id::pitchDecay, 0.34f }, { id::vcaDecay, 0.43f },
                                  { id::cutoff, 0.24f }, { id::resonance, 0.44f }, { id::vcfEg, 0.22f },
                                  { id::vcfDecay, 0.52f }, { id::pitchTarget, 1.0f } };
                velocities (v, { 1, 0.6f, 0.8f, 0.6f, 1, 0.6f, 0.8f, 0.7f });
                add ("FM Perc", v);
            }

            category = "BASS";
            add ("Acid Line", acid());
            add ("Sub Bass", subBass());
            {
                auto v = Values { { id::vco1Square, 1.0f }, { id::vco1Freq, clock (10.2f) }, { id::vco1Eg, bipolar (12.8f) },
                                  { id::vco2Level, 0.0f }, { id::noiseLevel, 0.0f }, { id::pitchDecay, clock (9) },
                                  { id::cutoff, clock (10) }, { id::resonance, clock (1.5f) }, { id::vcfEg, bipolar (2.5f) },
                                  { id::vcfDecay, clock (10.5f) }, { id::vcaDecay, clock (12) }, { id::scale, scalePentatonic } };
                pitches (v, { 0, 12, 0, 12, 3, 15, 0, 10 });
                chances (v, { 1, 0.8f, 1, 0.7f, 1, 0.8f, 1, 0.6f });
                add ("Bounce Bass", v);
            }
            {
                auto v = Values { { id::vco1Freq, clock (10.3f) }, { id::vco2Freq, clock (12.3f) }, { id::fmAmount, 0.45f },
                                  { id::vco1Level, 0.4f }, { id::vco2Level, 0.8f }, { id::vco2Square, 0.0f },
                                  { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f }, { id::noiseLevel, 0.0f },
                                  { id::cutoff, clock (12) }, { id::vcfEg, bipolar (1) }, { id::vcfDecay, clock (10) },
                                  { id::vcaDecay, clock (11) }, { id::scale, scaleMinor } };
                pitches (v, { 0, 0, 7, 0, 10, 0, 5, 3 });
                add ("FM Bass", v);
            }
            add ("Drift Bass", subBass() + Values { { id::analog, 1.0f }, { id::glide, clock (9.5f) } });
            {
                // matched to a sub bass of the original: two triangles, strong FM, the filter low
                auto v = Values { { id::vco1Freq, 0.351f }, { id::vco1Eg, -0.21f }, { id::vco1Level, 0.9f },
                                  { id::vco2Freq, 0.658f }, { id::vco2Eg, -0.3f }, { id::vco2Level, 0.69f },
                                  { id::fmAmount, 0.76f }, { id::noiseLevel, 0.0f }, { id::pitchDecay, 0.23f },
                                  { id::vcaDecay, 0.58f }, { id::cutoff, 0.31f }, { id::resonance, 0.22f },
                                  { id::vcfEg, -0.12f }, { id::vcfDecay, 0.73f }, { id::scale, scaleMinor } };
                pitches (v, { 0, 0, 12, 0, -2, 0, 7, -5 });
                add ("FM Sub", v);
            }

            category = "LEAD / FX";
            add ("Laser Zap", { { id::vco1Freq, clock (1) }, { id::vco1Eg, 1.0f },
                                { id::pitchDecay, clock (10) }, { id::vcaDecay, clock (10) },
                                { id::resonance, clock (2) } });
            {
                auto v = Values { { id::vco1Square, 1.0f }, { id::vco1Freq, clock (1) }, { id::vco1Eg, bipolar (12.5f) },
                                  { id::vco2Level, 0.0f }, { id::noiseLevel, 0.0f }, { id::pitchDecay, clock (9) },
                                  { id::vcaDecay, clock (9.5f) }, { id::cutoff, clock (3) }, { id::resonance, clock (10) },
                                  { id::scale, scalePentatonic } };
                pitches (v, { 12, 7, 19, 0, 14, 7, 24, 5 });
                chances (v, { 1, 0.7f, 1, 0.8f, 0.7f, 1, 0.6f, 0.9f });
                ratchets (v, { 1, 1, 2, 1, 1, 1, 2, 1 });
                add ("Bleeps", v);
            }
            {
                auto v = Values { { id::vco1Freq, clock (12.5f) }, { id::vco1Eg, 0.0f }, { id::vco2Level, 0.0f },
                                  { id::noiseLevel, 0.0f }, { id::glide, clock (11) }, { id::vcaSlow, 1.0f },
                                  { id::vcaDecay, clock (12) }, { id::cutoff, clock (1) }, { id::scale, scalePentatonic } };
                pitches (v, { 0, 12, -12, 24, 0, 7, -5, 19 });
                add ("Ping Pong", v);
            }
            add ("Alarm", { { id::vco1Square, 1.0f }, { id::vco2Square, 1.0f }, { id::vco1Freq, clock (12.5f) },
                            { id::vco2Freq, clock (12.6f) }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f }, { id::noiseLevel, 0.0f },
                            { id::lfoTarget, toPitch }, { id::lfoShape, square }, { id::lfoRate, 0.55f }, { id::lfoDepth, 0.45f },
                            { id::vcaSlow, 1.0f }, { id::vcaDecay, clock (2) }, { id::cutoff, clock (1.5f) },
                            { id::resonance, clock (12) }, { id::rate, quarter }, { id::length, 4.0f } });
            add ("Robot Talk", { { id::hardSync, 1.0f }, { id::vco1Freq, clock (10.5f) }, { id::vco2Freq, clock (12) },
                                 { id::vco1Eg, 0.0f }, { id::vco2Eg, 1.0f }, { id::vco1Level, 0.2f }, { id::vco2Level, 0.8f },
                                 { id::pitchDecay, clock (11.5f) }, { id::vcaDecay, clock (11.5f) }, { id::cutoff, clock (1) },
                                 { id::resonance, clock (11) }, { id::noiseLevel, 0.0f } });
            add ("Sweep Up", { { id::vco1Eg, bipolar (8) }, { id::pitchDecay, clock (12.5f) }, { id::vcaSlow, 1.0f },
                               { id::vcaDecay, clock (12.5f) }, { id::cutoff, clock (12) }, { id::vcfEg, bipolar (10) },
                               { id::vcfDecay, clock (12) }, { id::resonance, clock (12) }, { id::rate, eighth } });
            {
                auto v = Values { { id::vco1Level, 0.0f }, { id::vco2Level, 0.0f }, { id::noiseLevel, 1.0f }, { id::noisePink, 1.0f },
                                  { id::cutoff, clock (12) }, { id::vcaSlow, 1.0f }, { id::vcaDecay, clock (11.5f) },
                                  { id::lfoTarget, toCutoff }, { id::lfoShape, sampleHold }, { id::lfoSync, 1.0f },
                                  { id::lfoDivision, sixteenthNote }, { id::lfoDepth, 0.6f }, { id::rate, thirtySecond } };
                chances (v, { 1, 0.6f, 0.6f, 0.6f, 1, 0.6f, 0.6f, 0.6f });
                velocities (v, { 1, 0.5f, 0.7f, 0.4f, 0.9f, 0.6f, 0.5f, 0.8f });
                add ("Static Rain", v);
            }

            category = "GROOVE";
            add ("Moving Groove", { { id::lfoTarget, toCutoff }, { id::lfoShape, triangle }, { id::lfoSync, 1.0f },
                                    { id::lfoDivision, oneBar }, { id::lfoDepth, 0.5f } });
            add ("Random Pitches", { { id::lfoTarget, toPitch }, { id::lfoShape, sampleHold }, { id::lfoSync, 1.0f },
                                     { id::lfoDivision, sixteenthNote }, { id::lfoDepth, 0.2f } });
            {
                auto v = kick() + Values { { id::vcaDecay, clock (10.5f) } };
                ratchets (v, { 1, 1, 2, 1, 1, 3, 1, 4 });
                velocities (v, { 1, 0.6f, 0.8f, 0.6f, 1, 0.7f, 0.6f, 0.9f });
                add ("Ratchet Rolls", v, "trig>vcoReset");
            }
            {
                auto v = tom() + Values { { id::length, 5.0f }, { id::scale, scaleMinor } };
                pitches (v, { 0, 7, 3, 10, 5 });
                add ("Five Step", v, "trig>vcoReset");
            }

            // patchbay sounds
            category = "PATCHBAY";
            add ("Rolls (INT)", kick() + Values { { id::vcaDecay, clock (11) }, { id::tempo, 90.0f }, { id::rate, eighth } },
                 "vcaEg>tempo");
            add ("Growl", { { id::vco2Freq, clock (1) }, { id::noiseVcf, clock (1) }, { id::resonance, clock (1) },
                            { id::cutoff, clock (11) }, { id::vcaDecay, clock (11) } }, "vco2>vcfMod");
            {
                Values v { { id::vco1Freq, clock (9.5f) }, { id::vco1Eg, 0.0f }, { id::vco2Eg, 0.0f },
                           { id::scale, scaleMinor }, { id::glide, clock (10) },
                           { id::lfoTarget, toCutoff }, { id::lfoSync, 1.0f }, { id::lfoDivision, twoBars }, { id::lfoDepth, 0.45f } };
                pitches (v, { 0, 7, 3, 10, 0, 5, 12, 7 });
                velocities (v, { 0.9f, 0.5f, 0.7f, 0.5f, 0.9f, 0.6f, 0.8f, 0.5f });
                add ("Drone", v, "velocity>vcaCv");
            }
            add ("Feedback", { { id::noiseLevel, 0.4f }, { id::drive, 0.4f }, { id::resonance, clock (1) },
                               { id::vcaDecay, clock (11) } }, "vca>ext");
            add ("Melodic Filter", acid() + Values { { id::noiseVcf, clock (2) }, { id::vcfEg, bipolar (1) } }, "pitch>vcfMod");
            {
                auto v = Values { { id::vco1Level, 0.0f }, { id::vco2Level, 0.0f }, { id::noiseLevel, 0.0f },
                                  { id::resonance, 1.0f }, { id::cutoff, clock (11.5f) }, { id::vcfEg, bipolar (1) },
                                  { id::vcfDecay, clock (11) }, { id::vcaDecay, clock (11.5f) }, { id::noiseVcf, clock (1.5f) },
                                  { id::scale, scalePentatonic } };
                pitches (v, { 0, 7, 12, 3, 10, 7, 15, 5 });
                add ("Filter Ping", v, "pitch>vcfMod");
            }
            add ("Accent Filter", acid() + Values { { id::vcfDecay, clock (8.5f) } }, "velocity>vcfDecay");
            add ("VCO Clock", { { id::vco1Freq, 0.0f }, { id::vco1Square, 1.0f }, { id::vco1Level, 0.0f }, { id::vco1Eg, bipolar (2) },
                                { id::vco2Freq, clock (11) }, { id::vco2Level, 0.9f }, { id::vco2Square, 0.0f },
                                { id::vco2Eg, bipolar (2) }, { id::noiseLevel, 0.05f }, { id::vcaDecay, clock (10) } },
                 "vco1>advClock");
            add ("Chaos Engine", { { id::fmAmount, 0.3f }, { id::vco2Level, 0.6f }, { id::noiseVcf, 0.5f },
                                   { id::resonance, clock (12) }, { id::vcaDecay, clock (11) }, { id::analog, 0.7f } },
                 "vco2>vco1Cv vcaEg>vcfMod");

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
