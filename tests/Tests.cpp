// Offline tests for the PS-8 engine, sequencer and processor.

#include "FactoryPresets.h"
#include "Parameters.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "dsp/Engine.h"
#include "dsp/Sequencer.h"

#include <cmath>
#include <cstdio>
#include <string_view>
#include <vector>

namespace
{
    int failures = 0;

    void check (bool condition, const char* what)
    {
        std::printf ("%s  %s\n", condition ? "PASS" : "FAIL", what);

        if (! condition)
            ++failures;
    }

    std::vector<float> renderHit (const ps8::dsp::Settings& s, double sampleRate, double seconds,
                                  float velocity = 1.0f, float semitones = 0.0f)
    {
        ps8::dsp::Engine engine;
        engine.setSettings (s);
        engine.prepare (sampleRate);
        engine.trigger (velocity, semitones);

        std::vector<float> out ((size_t) (seconds * sampleRate));
        engine.render (out.data(), (int) out.size());
        return out;
    }

    float peak (const std::vector<float>& x, size_t from = 0, size_t to = SIZE_MAX)
    {
        float p = 0.0f;
        for (auto i = from; i < std::min (to, x.size()); ++i)
            p = std::max (p, std::abs (x[i]));
        return p;
    }

    double energy (const std::vector<float>& x, size_t from, size_t to)
    {
        double e = 0.0;
        for (auto i = from; i < std::min (to, x.size()); ++i)
            e += (double) x[i] * x[i];
        return e;
    }

    bool near (float a, float b) { return std::abs (a - b) < 1.0e-4f; }

    bool allFinite (const std::vector<float>& x)
    {
        for (auto v : x)
            if (! std::isfinite (v))
                return false;
        return true;
    }

    // zero crossings per second of the first part of a hit (rough pitch measure)
    double crossingRate (const std::vector<float>& x, double sampleRate, size_t from, size_t to)
    {
        int crossings = 0;
        for (auto i = from + 1; i < std::min (to, x.size()); ++i)
            if ((x[i - 1] < 0.0f) != (x[i] < 0.0f))
                ++crossings;
        return crossings * sampleRate / (double) (std::min (to, x.size()) - from);
    }

    void setParam (PS8AudioProcessor& processor, const juce::String& paramId, float plainValue)
    {
        auto* parameter = processor.getState().getParameter (paramId);
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plainValue));
    }

    float renderProcessor (PS8AudioProcessor& processor, juce::MidiBuffer midi, int blocks = 20)
    {
        juce::AudioBuffer<float> buffer (2, 512);
        float peakLevel = 0.0f;

        for (int i = 0; i < blocks; ++i)
        {
            buffer.clear();
            auto events = i == 0 ? midi : juce::MidiBuffer();
            processor.processBlock (buffer, events);
            peakLevel = std::max (peakLevel, buffer.getMagnitude (0, 0, buffer.getNumSamples()));
        }

        return peakLevel;
    }

    juce::MidiBuffer noteOn (int note, int position = 10)
    {
        juce::MidiBuffer midi;
        midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 120), position);
        return midi;
    }

    // Host transport for the processor tests
    struct TestPlayHead final : juce::AudioPlayHead
    {
        double ppq = 0.0, bpm = 120.0;
        bool playing = false;

        juce::Optional<PositionInfo> getPosition() const override
        {
            PositionInfo info;
            info.setBpm (bpm);
            info.setPpqPosition (ppq);
            info.setIsPlaying (playing);
            return info;
        }
    };
}

//==============================================================================
static void testEngine()
{
    const ps8::dsp::Settings init;

    for (const auto sampleRate : { 44100.0 * 4, 48000.0 * 4, 96000.0 * 2, 192000.0 })
    {
        const auto out = renderHit (init, sampleRate, 1.0);
        check (allFinite (out) && peak (out) > 0.05f && peak (out) < 1.5f, "init sound renders a sane hit");
    }

    {
        ps8::dsp::Engine engine;
        engine.setSettings (init);
        engine.prepare (192000.0);
        std::vector<float> out (19200);
        engine.render (out.data(), (int) out.size());
        check (peak (out) == 0.0f, "silent without a trigger");
    }

    {
        const auto loud = renderHit (init, 192000.0, 0.3, 1.0f);
        const auto soft = renderHit (init, 192000.0, 0.3, 0.4f);
        check (peak (soft) < 0.6f * peak (loud), "velocity scales the VCA");
    }

    {
        auto s = init;
        s.vcaDecay = 0.2f;
        const auto shortHit = renderHit (s, 192000.0, 1.0);
        s.vcaDecay = 0.7f;
        const auto longHit = renderHit (s, 192000.0, 1.0);
        check (energy (longHit, 38400, 192000) > 10.0 * energy (shortHit, 38400, 192000), "VCA decay sets the length");
    }

    {
        auto s = init;
        s.vco1Eg = s.vco2Eg = 0.0f;
        s.vco2Level = 0.0f;
        s.noiseLevel = 0.0f;
        s.cutoff = 1.0f;
        s.vcaDecay = 0.8f;
        const auto low = renderHit (s, 192000.0, 0.5, 1.0f, 0.0f);
        const auto high = renderHit (s, 192000.0, 0.5, 1.0f, 12.0f);
        const auto ratio = crossingRate (high, 192000.0, 0, 96000) / crossingRate (low, 192000.0, 0, 96000);
        check (ratio > 1.9 && ratio < 2.1, "step pitch +12 raises the VCOs by an octave");

        s.pitchTarget = ps8::dsp::PitchTarget::off;
        const auto unaffected = renderHit (s, 192000.0, 0.5, 1.0f, 12.0f);
        const auto same = crossingRate (unaffected, 192000.0, 0, 96000) / crossingRate (low, 192000.0, 0, 96000);
        check (same > 0.95 && same < 1.05, "seq pitch target off leaves the VCOs alone");
    }

    {
        ps8::dsp::Engine engine;
        auto s = init;
        s.vcaDecay = 1.0f;
        engine.setSettings (s);
        engine.prepare (192000.0);
        engine.trigger (1.0f, 0.0f);
        std::vector<float> out (19200);
        engine.render (out.data(), 9600);
        engine.panic();
        engine.render (out.data() + 9600, 9600);
        check (peak (out, 0, 9600) > 0.05f && peak (out, 9600 + 7680) < 1.0e-3f && engine.isSilent(), "SCRAM fades out within 40 ms");
    }

    // glide: the pitch slides to the new step instead of jumping
    {
        auto s = init;
        s.vco1Eg = s.vco2Eg = 0.0f;
        s.vco2Level = s.noiseLevel = 0.0f;
        s.cutoff = 1.0f;
        s.vcaDecay = 0.8f;

        auto secondStepRate = [&s] (float glide)
        {
            s.glide = glide;
            ps8::dsp::Engine engine;
            engine.setSettings (s);
            engine.prepare (192000.0);
            engine.trigger (1.0f, 0.0f);
            std::vector<float> out (192000);
            engine.render (out.data(), 96000);
            engine.trigger (1.0f, 12.0f);
            engine.render (out.data() + 96000, 96000);
            return crossingRate (out, 192000.0, 96000, 96000 + 9600);   // first 50 ms of the new step
        };

        const auto jump = secondStepRate (0.0f);
        const auto slide = secondStepRate (0.6f);
        check (slide < 0.8 * jump, "glide slides into the new step pitch");
    }

    // LFO on the volume: a slow square wave halves the level in its second half
    {
        auto s = init;
        s.vcaDecay = 1.0f;
        s.lfoTarget = ps8::dsp::LfoTarget::volume;
        s.lfoShape = ps8::dsp::LfoShape::square;
        s.lfoHz = 1.0f;
        s.lfoDepth = 1.0f;
        const auto out = renderHit (s, 48000.0 * 4, 1.0);
        const auto first = energy (out, 192000 / 10, 192000 * 4 / 10);
        const auto second = energy (out, 192000 * 6 / 10, 192000 * 9 / 10);
        s.lfoDepth = 0.0f;
        const auto plain = renderHit (s, 48000.0 * 4, 1.0);
        const auto plainSecond = energy (plain, 192000 * 6 / 10, 192000 * 9 / 10);
        check (first > 0.0 && second < 0.01 * plainSecond && allFinite (out), "LFO square on the volume gates the sound");
    }

    // extreme settings stay finite and bounded
    {
        auto s = init;
        s.resonance = 1.0f;
        s.cutoff = 0.4f;
        s.fmAmount = 1.0f;
        s.hardSync = true;
        s.vco1Freq = s.vco2Freq = 1.0f;
        s.vco1Eg = s.vco2Eg = 1.0f;
        s.vcfEg = 1.0f;
        s.noiseLevel = s.noiseVcf = 1.0f;
        s.drive = 1.0f;
        s.volume = 1.0f;
        s.vcaDecay = 0.8f;

        for (const auto highPass : { false, true })
            for (const auto pink : { false, true })
            {
                s.highPass = highPass;
                s.noisePink = pink;
                const auto out = renderHit (s, 88200.0, 1.0);
                check (allFinite (out) && peak (out) < 3.0f, "extreme settings stay finite and bounded");
            }

        s = init;
        s.resonance = 1.0f;
        s.vco1Level = s.vco2Level = s.noiseLevel = 0.0f;
        s.vcaDecay = 0.8f;
        const auto ringing = renderHit (s, 192000.0, 0.5);
        check (peak (ringing, 48000) > 0.01f, "full resonance self-oscillates");
    }
}

static void testSequencer()
{
    using namespace ps8::seq;
    std::array<Tick, 64> ticks;

    {
        Sequencer seq;
        seq.prepare (48000.0);
        std::vector<Tick> all;

        for (int block = 0; block < 200; ++block)
        {
            const auto n = seq.runInternal (500, 120.0, Rate::sixteenth, ticks.data(), 64);
            for (int i = 0; i < n; ++i)
                all.push_back ({ ticks[(size_t) i].position + block * 500, ticks[(size_t) i].step });
        }

        bool spacing = all.size() == 17, steps = true;   // 100000 samples / 6000 per sixteenth at 120 BPM

        for (size_t i = 0; i < all.size(); ++i)
        {
            spacing = spacing && std::abs (all[i].position - (int) i * 6000) <= 1;
            steps = steps && all[i].step == (int) i % 8;
        }

        check (! all.empty() && all[0].position == 0, "internal clock starts on the first sample");
        check (spacing, "internal clock: one step per sixteenth");
        check (steps, "internal clock cycles through the 8 steps");

        seq.reset();
        auto n = 0;
        for (int block = 0; block < 20 && n == 0; ++block)
            n = seq.runInternal (500, 120.0, Rate::sixteenth, ticks.data(), 64);
        check (n == 1 && ticks[0].step == 0, "reset: the next step is step 1");
    }

    {
        Sequencer seq;
        seq.prepare (48000.0);
        seq.setLength (5);
        bool ok = true;
        int count = 0;

        for (int block = 0; block < 400; ++block)
        {
            const auto n = seq.runInternal (500, 120.0, Rate::sixteenth, ticks.data(), 64);
            for (int i = 0; i < n; ++i, ++count)
                ok = ok && ticks[(size_t) i].step == count % 5;
        }

        check (ok && count > 10, "length 5 cycles through steps 1-5");

        seq.setLength (16);
        double ppq = 0.0;
        int last = -1;

        for (int block = 0; block < 300; ++block, ppq += 512 * 120.0 / 60.0 / 48000.0)
            if (seq.runHost (ppq, 120.0, 512, Rate::sixteenth, block == 0, ticks.data(), 64) > 0)
                last = std::max (last, ticks[0].step);

        check (last == 15, "length 16 on the host clock reaches step 16");
    }

    {
        check (near (quantise (3.4f, Scale::chromatic), 3.0f), "quantise chromatic rounds to semitones");
        check (near (quantise (6.2f, Scale::major), 7.0f) && near (quantise (13.4f, Scale::major), 14.0f), "quantise major");
        check (near (quantise (3.6f, Scale::minor), 3.0f) && near (quantise (-1.6f, Scale::minor), -2.0f), "quantise minor");
        check (near (quantise (1.0f, Scale::pentatonic), 0.0f) && near (quantise (8.0f, Scale::pentatonic), 7.0f), "quantise pentatonic");
        check (near (quantise (2.37f, Scale::off), 2.37f), "quantise off keeps the pitch");
        check (near (stepPitch (0.0f, Scale::off, 2), 2.0f) && near (stepPitch (0.4f, Scale::off, 0), 0.4f),
               "root transposes the steps, also without a scale (step 0 = root)");
        check (near (stepPitch (1.2f, Scale::major, 2), 4.0f) && near (stepPitch (3.6f, Scale::minor, 9), 12.0f),
               "the scale is built on the root (D major: +1.2 -> E; A minor: +3.6 -> C)");
    }

    {
        Sequencer seq;
        seq.prepare (48000.0);
        const auto ppqPerSample = 120.0 / 60.0 / 48000.0;
        std::vector<Tick> all;
        double ppq = 0.0;

        for (int block = 0; block < 100; ++block)
        {
            const auto n = seq.runHost (ppq, 120.0, 512, Rate::sixteenth, block == 0, ticks.data(), 64);
            for (int i = 0; i < n; ++i)
                all.push_back ({ ticks[(size_t) i].position + block * 512, ticks[(size_t) i].step });
            ppq += 512 * ppqPerSample;
        }

        bool ok = all.size() == 9;   // 51200 samples: sixteenths at 0, 6000, …, 48000

        for (size_t i = 0; i < all.size() && ok; ++i)
            ok = std::abs (all[i].position - (int) i * 6000) <= 1 && all[i].step == (int) i % 8;

        check (ok, "host clock: steps on the sixteenth grid from the song start");

        // locate into the middle of a bar: steps follow the bar position
        const auto n = seq.runHost (1.25, 120.0, 512, Rate::sixteenth, false, ticks.data(), 64);
        check (n == 1 && ticks[0].position == 0 && ticks[0].step == 5, "host clock follows a jump (beat 2.25 = step 6)");

        const auto eighths = seq.runHost (4.0, 120.0, 512, Rate::eighth, true, ticks.data(), 64);
        check (eighths == 1 && ticks[0].step == 0, "host clock: bar start with 1/8 is step 1");
    }
}

static void testProcessor()
{
    namespace id = ps8::params::id;

    PS8AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, 48000.0, 512);
    setParam (processor, id::hostClock, 0.0f);
    setParam (processor, id::run, 0.0f);
    processor.prepareToPlay (48000.0, 512);

    auto settle = [&processor] { renderProcessor (processor, {}, 600); };

    check (renderProcessor (processor, {}, 50) < 1.0e-4f, "stopped sequencer is silent");

    check (renderProcessor (processor, noteOn (PS8AudioProcessor::firstStepNote + 2)) > 0.05f, "MIDI note E1 plays a step");
    check (processor.getDisplay().step.load() == 2, "MIDI note E1 selects step 3");
    settle();

    setParam (processor, id::stepOn (4), 0.0f);
    check (renderProcessor (processor, noteOn (PS8AudioProcessor::firstStepNote + 4)) < 1.0e-4f, "switched-off step stays silent");
    setParam (processor, id::stepOn (4), 1.0f);
    settle();

    // the last note selected step 5
    const auto advanced = renderProcessor (processor, noteOn (PS8AudioProcessor::advanceNote));
    check (advanced > 0.02f && processor.getDisplay().step.load() == 5, "ADV note plays the next step");
    settle();

    check (renderProcessor (processor, noteOn (PS8AudioProcessor::firstStepNote + 9)) > 0.02f && processor.getDisplay().step.load() == 9,
           "MIDI note A1 plays step 10");
    settle();

    setParam (processor, id::run, 1.0f);
    check (renderProcessor (processor, {}, 50) > 0.05f, "running internal clock plays");
    setParam (processor, id::run, 0.0f);
    settle();

    // host clock: plays only while the transport runs
    TestPlayHead playHead;
    processor.setPlayHead (&playHead);
    setParam (processor, id::hostClock, 1.0f);
    setParam (processor, id::run, 1.0f);
    check (renderProcessor (processor, {}, 50) < 1.0e-4f, "host clock: silent while the transport is stopped");

    playHead.playing = true;
    juce::AudioBuffer<float> buffer (2, 512);
    float level = 0.0f;

    for (int i = 0; i < 50; ++i)
    {
        juce::MidiBuffer none;
        buffer.clear();
        processor.processBlock (buffer, none);
        level = std::max (level, buffer.getMagnitude (0, 512));
        playHead.ppq += 512 * 120.0 / 60.0 / 48000.0;
    }

    check (level > 0.05f, "host clock: plays while the transport runs");
    processor.setPlayHead (nullptr);

    // state round trip
    setParam (processor, id::cutoff, 0.123f);
    juce::MemoryBlock block;
    processor.getStateInformation (block);
    setParam (processor, id::cutoff, 0.9f);
    processor.setStateInformation (block.getData(), (int) block.getSize());
    check (std::abs (processor.getState().getRawParameterValue (id::cutoff)->load() - 0.123f) < 1.0e-3f, "state round trip");
}

// HOST clock as in a DAW: the transport already plays mid-song, then RUN is switched on
static void testHostStart()
{
    namespace id = ps8::params::id;
    constexpr double sampleRate = 44100.0;
    constexpr int blockSize = 441;

    PS8AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, sampleRate, blockSize);
    processor.prepareToPlay (sampleRate, blockSize);

    TestPlayHead playHead;
    playHead.playing = true;
    playHead.ppq = 13.37;
    playHead.bpm = 120.0;
    processor.setPlayHead (&playHead);

    juce::AudioBuffer<float> buffer (2, blockSize);
    auto play = [&] (int blocks, int& steps, float& level)
    {
        int last = processor.getDisplay().step.load();

        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer none;
            buffer.clear();
            processor.processBlock (buffer, none);
            level = std::max (level, buffer.getMagnitude (0, blockSize));
            playHead.ppq += blockSize * playHead.bpm / 60.0 / sampleRate;

            if (const auto step = processor.getDisplay().step.load(); step != last)
                ++steps, last = step;
        }
    };

    int steps = 0;
    float level = 0.0f;

    // HOST without any DAW clock (like the Standalone app) waits instead of running on its own
    processor.setPlayHead (nullptr);
    setParam (processor, id::run, 1.0f);
    play (100, steps, level);
    check (steps == 0 && level < 1.0e-4f && ! processor.getDisplay().running.load(), "HOST without a DAW clock stays stopped");
    setParam (processor, id::run, 0.0f);
    processor.setPlayHead (&playHead);

    play (20, steps, level);
    check (steps == 0 && level < 1.0e-4f, "host clock, RUN off: silent while the DAW plays");

    setParam (processor, id::run, 1.0f);
    steps = 0;
    play (200, steps, level);   // 2 s = 16 sixteenths at 120 BPM
    check (steps >= 15 && steps <= 17 && level > 0.05f && processor.getDisplay().running.load(),
           "host clock: switching RUN on while the DAW plays starts the sequencer");
    processor.setPlayHead (nullptr);
}

//==============================================================================
static void testPatch()
{
    namespace id = ps8::params::id;
    using namespace ps8::patch;
    const ps8::dsp::Settings init;

    auto withCables = [] (std::initializer_list<std::pair<Output, Input>> cables)
    {
        Masks masks {};
        for (const auto& [o, in] : cables)
            masks[(size_t) in] |= (Mask) (1u << o);
        return masks;
    };

    auto renderPatched = [] (const ps8::dsp::Settings& s, const Masks& masks, double seconds, float semitones = 0.0f)
    {
        ps8::dsp::Engine engine;
        engine.setSettings (s);
        engine.prepare (192000.0);
        engine.setPatch (masks);
        engine.trigger (1.0f, semitones);
        std::vector<float> out ((size_t) (seconds * 192000.0));
        engine.render (out.data(), (int) out.size());
        return out;
    };

    {
        const auto plain = renderHit (init, 192000.0, 0.5);
        const auto unpatched = renderPatched (init, {}, 0.5);
        check (plain == unpatched, "no cables: the sound is unchanged");
    }

    // VCA CV is normalled to the VCA envelope: a cable holding it open keeps the sound going
    {
        auto s = init;
        s.vcaDecay = 0.2f;
        const auto plain = renderPatched (s, {}, 1.0);
        const auto held = renderPatched (s, withCables ({ { velocityOut, vcaCvIn } }), 1.0);
        check (energy (held, 96000, 192000) > 100.0 * energy (plain, 96000, 192000) && energy (held, 96000, 192000) > 1.0,
               "VELOCITY into VCA CV holds the VCA open");
    }

    // PITCH out into a VCO CV input tracks 1 V/oct
    {
        auto s = init;
        s.vco1Eg = s.vco2Eg = 0.0f;
        s.vco2Level = s.noiseLevel = 0.0f;
        s.cutoff = 1.0f;
        s.vcaDecay = 0.8f;
        s.pitchTarget = ps8::dsp::PitchTarget::off;
        const auto low = renderPatched (s, withCables ({ { pitchOut, vco1CvIn } }), 0.5, 0.0f);
        const auto high = renderPatched (s, withCables ({ { pitchOut, vco1CvIn } }), 0.5, 12.0f);
        const auto ratio = crossingRate (high, 192000.0, 0, 96000) / crossingRate (low, 192000.0, 0, 96000);
        check (ratio > 1.9 && ratio < 2.1, "PITCH into VCO 1 CV: +12 steps raise VCO 1 by an octave");
    }

    // like the original: softer hits decay a little faster
    {
        auto s = init;
        s.vcaDecay = 0.6f;
        const auto loud = renderHit (s, 192000.0, 0.8, 1.0f);
        const auto soft = renderHit (s, 192000.0, 0.8, 0.5f);
        const auto tail = [] (const std::vector<float>& x) { return energy (x, 57600, 115200) / energy (x, 0, 9600); };
        check (tail (soft) < 0.8 * tail (loud), "softer hits decay faster");
    }

    // VCO RST: TRIG into it starts every hit at the same oscillator phase
    {
        auto s = init;
        s.vcaDecay = 0.3f;
        s.noiseLevel = 0.0f;   // noise differs on every hit anyway

        auto hitDifference = [&s] (const Masks& masks)
        {
            ps8::dsp::Engine engine;
            engine.setSettings (s);
            engine.prepare (192000.0);
            engine.setPatch (masks);

            // two hits 100000 samples apart: not a whole number of VCO 1 periods
            std::vector<float> idle (100000), first (9600), second (9600);
            engine.render (idle.data(), (int) idle.size());
            engine.trigger (1.0f, 0.0f);
            engine.render (first.data(), (int) first.size());
            engine.render (idle.data(), (int) idle.size() - (int) first.size());
            engine.trigger (1.0f, 0.0f);
            engine.render (second.data(), (int) second.size());

            // from 2.5 ms on: the filter needs a moment to forget what played before the hit
            float difference = 0.0f;
            for (size_t i = 480; i < first.size(); ++i)
                difference = std::max (difference, std::abs (first[i] - second[i]));
            return difference / peak (first);
        };

        const auto freeRunning = hitDifference ({});
        const auto reset = hitDifference (withCables ({ { trigOut, vcoResetIn } }));
        check (freeRunning > 0.3f && reset < 0.01f, "TRIG into VCO RST makes every hit the same (free running: they differ)");
    }

    // EXT IN with a cable replaces the noise in the mixer
    {
        auto s = init;
        s.vco1Level = s.vco2Level = 0.0f;
        s.noiseLevel = 1.0f;
        s.vcaDecay = 0.6f;
        const auto noise = renderPatched (s, {}, 0.5);
        const auto silentExt = renderPatched (s, withCables ({ { trigOut, extIn } }), 0.5);
        check (energy (silentExt, 19200, 96000) < 0.001 * energy (noise, 19200, 96000), "a cable in EXT IN replaces the noise");
    }

    // TRIG in fires the envelopes
    {
        ps8::dsp::Engine engine;
        engine.setSettings (init);
        engine.prepare (192000.0);
        engine.setPatch (withCables ({ { vco1Out, trigIn } }));
        std::vector<float> out (96000);
        engine.render (out.data(), (int) out.size());
        check (peak (out) > 0.05f, "a square into TRIG fires the envelopes");
    }

    // feedback patches stay finite
    {
        auto s = init;
        s.resonance = 1.0f;
        s.fmAmount = 1.0f;
        s.noiseVcf = 1.0f;
        s.vcaDecay = 0.8f;
        const auto out = renderPatched (s, withCables ({ { vcaOut, extIn }, { vco2Out, vco1CvIn }, { vcaOut, vcfModIn },
                                                         { vco1Out, fmAmountIn }, { vcaEgOut, vcaDecayIn }, { vcfEgOut, vcoDecayIn },
                                                         { vco2Out, trigIn }, { vco1Out, vcaCvIn }, { vcaOut, noiseLevelIn } }), 1.0);
        check (allFinite (out) && peak (out) < 3.0f, "feedback patches stay finite and bounded");
    }

    {
        const auto masks = withCables ({ { vcaEgOut, tempoIn }, { vco2Out, vcfModIn }, { vco1Out, vcfModIn }, { trigOut, resetIn } });
        check (PS8AudioProcessor::patchFromString (PS8AudioProcessor::patchToString (masks)) == masks
                   && PS8AudioProcessor::patchFromString ("bogus>tempo vca>nothing") == Masks {},
               "patch text round trip");
    }

    // sequencer jacks
    PS8AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, 48000.0, 512);
    setParam (processor, id::hostClock, 0.0f);
    setParam (processor, id::tempo, 20.0f);
    setParam (processor, id::rate, 0.0f);   // quarters: one step every 3 s
    setParam (processor, id::run, 1.0f);
    processor.prepareToPlay (48000.0, 512);

    auto countSteps = [&processor] (int blocks)
    {
        juce::AudioBuffer<float> buffer (2, 512);
        int changes = 0, last = processor.getDisplay().step.load();

        for (int i = 0; i < blocks; ++i)
        {
            juce::MidiBuffer none;
            buffer.clear();
            processor.processBlock (buffer, none);

            if (const auto step = processor.getDisplay().step.load(); step != last)
                ++changes, last = step;
        }

        return changes;
    };

    // start from step 1 with a fresh clock
    auto restart = [&] (int blocks)
    {
        setParam (processor, id::run, 0.0f);
        countSteps (1);
        setParam (processor, id::run, 1.0f);
        return countSteps (blocks);
    };

    const auto plainSteps = restart (188);   // 2 s

    setParam (processor, id::vco1Freq, 0.0f);   // 8 Hz square, no pitch envelope
    setParam (processor, id::vco1Eg, 0.0f);
    setParam (processor, id::vco1Square, 1.0f);
    processor.connect (vco1Out, advClockIn);
    const auto clocked = restart (188);
    check (plainSteps <= 1 && clocked >= 12 && clocked <= 20, "a square into ADV/CLK clocks the sequencer");
    processor.clearPatch();

    setParam (processor, id::vcaDecay, 0.8f);
    processor.connect (vcaEgOut, tempoIn);
    const auto faster = restart (188);
    check (faster > plainSteps + 2, "VCA EG into TEMPO speeds up the clock");
    processor.clearPatch();

    // PITCH out is 0 V with the init pitches: a low RUN input stops the sequencer
    processor.connect (pitchOut, runIn);
    renderProcessor (processor, {}, 400);
    check (renderProcessor (processor, {}, 200) < 1.0e-4f && ! processor.getDisplay().running.load(), "a low RUN input stops the sequencer");
    processor.clearPatch();

    // every step resets: the sequencer stays on step 1
    setParam (processor, id::tempo, 300.0f);
    setParam (processor, id::rate, 3.0f);
    processor.connect (trigOut, resetIn);
    countSteps (20);
    bool stuck = true;

    for (int i = 0; i < 100; ++i)
    {
        countSteps (1);
        stuck = stuck && processor.getDisplay().step.load() == 0;
    }

    check (stuck, "TRIG into RESET keeps the sequencer on step 1");

    // the patch is saved with the state
    processor.clearPatch();
    processor.connect (vcfEgOut, vco2CvIn);
    processor.connect (vcaOut, extIn);
    juce::MemoryBlock block;
    processor.getStateInformation (block);
    const auto version = processor.getPatchVersion();
    processor.clearPatch();
    processor.setStateInformation (block.getData(), (int) block.getSize());
    check (processor.isConnected (vcfEgOut, vco2CvIn) && processor.isConnected (vcaOut, extIn)
               && ! processor.isConnected (trigOut, resetIn) && processor.getPatchVersion() != version,
           "patch state round trip");

    // host audio into EXT IN replaces the noise
    {
        PS8AudioProcessor sidechained;
        sidechained.enableAllBuses();
        sidechained.setPlayConfigDetails (2, 2, 48000.0, 512);
        setParam (sidechained, id::run, 0.0f);
        setParam (sidechained, id::vco1Level, 0.0f);
        setParam (sidechained, id::vco2Level, 0.0f);
        setParam (sidechained, id::noiseLevel, 1.0f);
        sidechained.prepareToPlay (48000.0, 512);

        auto level = [&sidechained] (float amplitude)
        {
            juce::AudioBuffer<float> buffer (2, 512);
            float p = 0.0f;

            for (int b = 0; b < 20; ++b)
            {
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < 512; ++i)
                        buffer.setSample (c, i, amplitude * std::sin (0.0577f * (float) (b * 512 + i)));

                auto midi = b == 0 ? noteOn (PS8AudioProcessor::firstStepNote) : juce::MidiBuffer();
                sidechained.processBlock (buffer, midi);
                p = std::max (p, buffer.getMagnitude (0, 0, 512));
            }

            renderProcessor (sidechained, {}, 400);
            return p;
        };

        const auto silent = level (0.0f);
        const auto loud = level (0.8f);
        check (silent < 1.0e-3f && loud > 0.05f, "host audio into EXT IN replaces the noise");
    }
}

// The patchbay: drag from a jack to another to patch, double-click to pull the cables.
static void testPatchbayMouse()
{
    using namespace ps8::patch;
    PS8AudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    ps8::ui::Patchbay* patchbay = nullptr;
    std::function<void (juce::Component&)> find = [&] (juce::Component& c)
    {
        if (auto* p = dynamic_cast<ps8::ui::Patchbay*> (&c))
            patchbay = p;
        for (auto* child : c.getChildren())
            find (*child);
    };
    find (*editor);

    if (patchbay == nullptr)
    {
        check (false, "patchbay found");
        return;
    }

    auto source = juce::Desktop::getInstance().getMainMouseSource();
    auto event = [&] (juce::Point<float> position, int clicks = 1, juce::ModifierKeys mods = juce::ModifierKeys::leftButtonModifier)
    {
        return juce::MouseEvent (source, position, mods, juce::MouseInputSource::defaultPressure, 0.0f, 0.0f, 0.0f, 0.0f,
                                 patchbay, patchbay, juce::Time::getCurrentTime(), position, juce::Time::getCurrentTime(), clicks, false);
    };

    auto drag = [&] (juce::Point<float> from, juce::Point<float> to)
    {
        patchbay->mouseDown (event (from));
        patchbay->mouseDrag (event ((from + to) * 0.5f));
        patchbay->mouseDrag (event (to));
        patchbay->mouseUp (event (to));
    };

    using Patchbay = ps8::ui::Patchbay;
    const auto vcaEg = Patchbay::jackFor (vcaEgOut).centre;
    const auto tempo = Patchbay::jackFor (tempoIn).centre;
    const auto vcfMod = Patchbay::jackFor (vcfModIn).centre;

    check (patchbay->hitTest ((int) tempo.x, (int) tempo.y) && ! patchbay->hitTest (600, 300), "patchbay only takes clicks on its jacks");

    drag (vcaEg, tempo);
    check (processor.isConnected (vcaEgOut, tempoIn), "drag from VCA EG to TEMPO patches a cable");

    drag (vcfMod, vcaEg);
    check (processor.isConnected (vcaEgOut, vcfModIn), "dragging from an input to an output works too");

    drag (tempo, Patchbay::jackFor (resetIn).centre);
    check (! processor.isConnected (vcaEgOut, tempoIn) && processor.isConnected (vcaEgOut, resetIn), "a plug moves to another input");

    drag (Patchbay::jackFor (resetIn).centre, { 600.0f, 300.0f });
    check (! processor.isConnected (vcaEgOut, resetIn), "a plug dropped off the patchbay is pulled out");

    drag (vcaEg, Patchbay::jackFor (vco2Out).centre);
    check (processor.getPatch()[(size_t) vcfModIn] == (1u << vcaEgOut) && processor.getPatch()[(size_t) tempoIn] == 0,
           "output to output does not patch");

    patchbay->mouseDoubleClick (event (vcaEg, 2));
    check (processor.getPatch() == Masks {}, "double-click pulls all cables of a jack");
}

//==============================================================================
static void testPresets()
{
    namespace id = ps8::params::id;
    using Presets = ps8::Presets;

    PS8AudioProcessor processor;
    auto& presets = processor.getPresets();
    auto value = [&processor] (const juce::String& paramId) { return processor.getState().getRawParameterValue (paramId)->load(); };

    check (presets.current().name == "Init" && ! presets.isModified(), "a new instance starts with Init");
    processor.setPlayConfigDetails (0, 2, 48000.0, 512);
    processor.prepareToPlay (48000.0, 512);
    check (value (id::run) < 0.5f && renderProcessor (processor, {}, 20) == 0.0f, "a new instance starts with the sequencer stopped");

    // every factory preset names real parameters and loads
    {
        bool idsOk = true, loads = true;

        for (const auto& preset : ps8::factoryPresets())
        {
            for (const auto& [paramId, v] : preset.values)
                idsOk = idsOk && processor.getState().getParameter (paramId) != nullptr;

            loads = loads && presets.load ({ preset.name, true, {} }) && presets.current().name == preset.name && ! presets.isModified();
        }

        check (ps8::factoryPresets().size() >= 12 && idsOk && loads, "factory presets load and use real parameters");
    }

    presets.load ({ "Growl", true, {} });
    check (processor.isConnected (ps8::patch::vco2Out, ps8::patch::vcfModIn), "a factory preset brings its cables");
    presets.loadInit();
    check (processor.getPatch() == ps8::patch::Masks {} && std::abs (value (id::cutoff) - ps8::dsp::Settings().cutoff) < 1.0e-4f,
           "Init resets sound and cables");

    // save, change, load
    setParam (processor, id::cutoff, 0.234f);
    setParam (processor, id::length, 11.0f);
    processor.connect (ps8::patch::vcaEgOut, ps8::patch::tempoIn);
    check (presets.isModified(), "changing a knob marks the preset as modified");

    check (presets.save ("My Kick: v1") && Presets::userPresetExists ("My Kick: v1") && ! presets.isModified()
               && presets.current().name == "My Kick: v1" && ! presets.current().factory,
           "save a user preset (any name, also with characters not allowed in file names)");
    check (Presets::folder().isAChildOf (ps8::appDataFolder()) && Presets::fileFor ("My Kick: v1").existsAsFile(),
           "user presets live in the app data folder");

    setParam (processor, id::cutoff, 0.8f);
    setParam (processor, id::run, 1.0f);
    processor.clearPatch();
    presets.loadInit();

    const auto user = presets.userPresets();
    check (user.size() == 1 && presets.load (user[0]) && std::abs (value (id::cutoff) - 0.234f) < 1.0e-4f
               && juce::roundToInt (value (id::length)) == 11 && processor.isConnected (ps8::patch::vcaEgOut, ps8::patch::tempoIn),
           "load a user preset with its cables");
    check (value (id::run) > 0.5f, "loading a preset leaves RUN alone");

    // stepping wraps around Init / factory / user
    {
        const auto all = presets.list();
        presets.step (1);
        const auto wrapped = presets.current() == all.front();
        presets.step (-1);
        check (wrapped && presets.current() == all.back(), "< and > step through all presets and wrap around");
    }

    check (presets.rename (presets.current(), "Kick Two") && presets.current().name == "Kick Two"
               && ! Presets::userPresetExists ("My Kick: v1") && Presets::userPresetExists ("Kick Two"),
           "rename a user preset");
    check (! presets.rename ({ "Snare", true, {} }, "Mine") && ! presets.remove ({ "Snare", true, {} }), "factory presets are read-only");

    // the preset name travels with the project; a loaded project is not "modified"
    {
        juce::MemoryBlock block;
        processor.getStateInformation (block);

        PS8AudioProcessor other;
        other.setStateInformation (block.getData(), (int) block.getSize());
        check (other.getPresets().current().name == "Kick Two" && ! other.getPresets().current().factory
                   && ! other.getPresets().isModified(),
               "the project remembers the preset name");
    }

    check (presets.remove (presets.current()) && presets.userPresets().empty(), "delete a user preset");

    // window size
    {
        {
            ps8::UserSettings settings;
            settings.setEditorWidth (888);
        }

        ps8::UserSettings again;
        check (again.getEditorWidth (0) == 888 && ps8::appDataFolder().getChildFile ("settings.xml").existsAsFile(),
               "the window size is remembered per user");

        PS8AudioProcessor fresh;
        std::unique_ptr<juce::AudioProcessorEditor> editor (fresh.createEditor());
        check (editor->getWidth() == 888, "a new window opens in the remembered size");
    }
}

//==============================================================================
// Pattern memory and step tools
static void testPatterns()
{
    namespace id = ps8::params::id;
    using Randomise = ps8::Patterns::Randomise;

    PS8AudioProcessor processor;
    auto& patterns = processor.getPatterns();
    auto value = [&processor] (const juce::String& paramId) { return processor.getState().getRawParameterValue (paramId)->load(); };
    auto semitones = [&value] (int step) { return value (id::stepPitch (step)) * 24.0f; };
    auto setSemitones = [&processor] (int step, float st) { setParam (processor, id::stepPitch (step), st / 24.0f); };

    bool empty = patterns.current() == 0;
    for (int i = 0; i < ps8::Patterns::numPatterns; ++i)
        empty = empty && ! patterns.hasContent (i);
    check (empty, "a new instance plays pattern 1 and all eight patterns are empty");

    // pattern 1: a little melody over five steps
    for (int s = 0; s < 5; ++s)
        setSemitones (s, (float) (s + 1));
    setParam (processor, id::length, 5.0f);
    check (patterns.hasContent (0), "editing the steps fills the current pattern");

    patterns.select (1);
    check (patterns.current() == 1 && near (semitones (0), 0.0f) && juce::roundToInt (value (id::length)) == 8,
           "switching to an empty pattern loads its (Init) steps and length");
    setSemitones (2, -7.0f);

    patterns.select (0);
    check (near (semitones (0), 1.0f) && near (semitones (4), 5.0f) && juce::roundToInt (value (id::length)) == 5,
           "switching back brings pattern 1 back");
    patterns.select (1);
    check (near (semitones (2), -7.0f), "and pattern 2 kept its edit");
    patterns.select (0);

    patterns.copy (0, 3);
    patterns.select (3);
    check (near (semitones (1), 2.0f) && juce::roundToInt (value (id::length)) == 5, "copy a pattern to another slot");
    patterns.clear (3);
    check (! patterns.hasContent (3) && near (semitones (1), 0.0f), "clear the current pattern");
    patterns.select (0);

    // shift rotates within LENGTH; steps behind it stay where they are
    setSemitones (7, 9.0f);
    patterns.shift (-1);
    check (near (semitones (0), 2.0f) && near (semitones (3), 5.0f) && near (semitones (4), 1.0f) && near (semitones (7), 9.0f),
           "shift left rotates the steps within the length");
    patterns.shift (1);
    check (near (semitones (0), 1.0f) && near (semitones (4), 5.0f), "shift right turns it back");

    // copy / paste a page of eight steps
    setParam (processor, id::stepRatchet (1), 3.0f);
    patterns.copyPage (0);
    patterns.pastePage (1);
    check (patterns.canPaste() && near (semitones (8), 1.0f) && near (semitones (12), 5.0f)
               && juce::roundToInt (value (id::stepRatchet (9))) == 3,
           "copy page 1-8 and paste it into 9-16");

    // randomise only what was asked for, only within LENGTH, in whole semitones
    {
        const auto velocityBefore = value (id::stepVelocity (2));
        const auto pitchBehind = semitones (6);
        juce::Random random (1234);
        patterns.randomise (Randomise::pitch, random);

        bool whole = true, changed = false;
        for (int s = 0; s < 5; ++s)
        {
            whole = whole && near (semitones (s), std::round (semitones (s))) && std::abs (semitones (s)) <= 12.0f;
            changed = changed || ! near (semitones (s), (float) (s + 1));
        }

        check (whole && changed && near (value (id::stepVelocity (2)), velocityBefore) && near (semitones (6), pitchBehind),
               "RAND pitch: whole semitones within the length, nothing else touched");

        patterns.undo();
        check (near (semitones (0), 1.0f) && near (semitones (4), 5.0f), "UNDO takes the last tool back");
        patterns.undo();
        check (! near (semitones (0), 1.0f) || ! near (semitones (1), 2.0f) || ! near (semitones (2), 3.0f), "UNDO again redoes it");
        patterns.undo();

        juce::Random again (99);
        patterns.randomise (Randomise::steps, again);
        check (value (id::stepOn (0)) > 0.5f, "RAND steps keeps step 1 on");
        patterns.undo();
    }

    // all eight patterns travel with the project ...
    {
        patterns.select (2);
        setSemitones (0, 11.0f);

        juce::MemoryBlock block;
        processor.getStateInformation (block);

        PS8AudioProcessor other;
        other.setStateInformation (block.getData(), (int) block.getSize());
        auto& otherPatterns = other.getPatterns();
        const auto otherPitch = [&other] (int step) { return other.getState().getRawParameterValue (id::stepPitch (step))->load() * 24.0f; };

        const auto restored = otherPatterns.current() == 2 && near (otherPitch (0), 11.0f);
        otherPatterns.select (0);
        check (restored && near (otherPitch (0), 1.0f) && near (otherPitch (4), 5.0f) && otherPatterns.hasContent (1),
               "the project saves all patterns and the one that plays");
    }

    // ... and with a preset; Init and older presets start with empty patterns
    {
        auto& presets = processor.getPresets();
        presets.save ("Pattern Test");
        presets.loadInit();
        check (patterns.current() == 0 && ! patterns.hasContent (0) && ! patterns.hasContent (1), "Init empties all patterns");

        const auto user = presets.userPresets();
        const auto it = std::find_if (user.begin(), user.end(), [] (const auto& e) { return e.name == "Pattern Test"; });
        check (it != user.end() && presets.load (*it) && patterns.current() == 2 && near (semitones (0), 11.0f)
                   && patterns.hasContent (0) && patterns.hasContent (1),
               "a preset brings back all its patterns");

        patterns.select (0);
        check (presets.isModified(), "switching the pattern counts as a change to the preset");
        presets.remove (presets.current());
    }

    // the keys in the title bar
    {
        std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
        ps8::ui::SequencerTools* tools = nullptr;
        std::function<void (juce::Component&)> find = [&] (juce::Component& c)
        {
            if (auto* t = dynamic_cast<ps8::ui::SequencerTools*> (&c))
                tools = t;
            for (auto* child : c.getChildren())
                find (*child);
        };
        find (*editor);

        bool ok = tools != nullptr;

        if (ok)
        {
            tools->press (ps8::ui::SequencerTools::Key::pattern, 5);
            ok = patterns.current() == 5;
            tools->press (ps8::ui::SequencerTools::Key::pattern, 0);
            tools->press (ps8::ui::SequencerTools::Key::shiftLeft);
            ok = ok && patterns.current() == 0 && near (semitones (0), 2.0f) && patterns.canUndo();
            tools->press (ps8::ui::SequencerTools::Key::undo);
            ok = ok && near (semitones (0), 1.0f);
        }

        check (ok, "pattern keys and step tools in the sequencer title bar");
    }
}

static void testDoubleClick()
{
    PS8AudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());

    std::vector<juce::Slider*> sliders;
    std::function<void (juce::Component&)> collect = [&] (juce::Component& c)
    {
        if (auto* s = dynamic_cast<juce::Slider*> (&c))
            sliders.push_back (s);
        for (auto* child : c.getChildren())
            collect (*child);
    };
    collect (*editor);

    auto source = juce::Desktop::getInstance().getMainMouseSource();
    int enabled = 0, reset = 0;

    for (auto* slider : sliders)
    {
        if (slider->isDoubleClickReturnEnabled())
            ++enabled;

        const auto defaultValue = slider->getDoubleClickReturnValue();
        slider->setValue (slider->getMinimum() + 0.37 * (slider->getMaximum() - slider->getMinimum()));
        const auto centre = slider->getLocalBounds().getCentre().toFloat();
        const juce::MouseEvent e (source, centre, {}, juce::MouseInputSource::defaultPressure, 0.0f, 0.0f, 0.0f, 0.0f,
                                  slider, slider, juce::Time::getCurrentTime(), centre, juce::Time::getCurrentTime(), 2, false);
        slider->mouseDoubleClick (e);

        if (std::abs (slider->getValue() - defaultValue) < 1.0e-6)
            ++reset;
    }

    std::printf ("      %d sliders, %d with double-click, %d reset\n", (int) sliders.size(), enabled, reset);
    check (! sliders.empty() && reset == (int) sliders.size(), "double-click resets every knob to its default");
}

//==============================================================================
static void testRatchetAndChance()
{
    namespace id = ps8::params::id;

    PS8AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, 48000.0, 512);
    setParam (processor, id::hostClock, 0.0f);
    setParam (processor, id::run, 0.0f);
    setParam (processor, id::vcaDecay, 0.0f);   // 3 ms: every hit is a separate burst
    processor.prepareToPlay (48000.0, 512);

    auto render = [&processor] (int blocks, juce::MidiBuffer midi = {})
    {
        std::vector<float> out;
        juce::AudioBuffer<float> buffer (2, 512);

        for (int i = 0; i < blocks; ++i)
        {
            auto events = i == 0 ? midi : juce::MidiBuffer();
            buffer.clear();
            processor.processBlock (buffer, events);
            out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
        }

        return out;
    };

    // onsets: loud after at least 400 quiet samples
    auto onsets = [] (const std::vector<float>& x)
    {
        std::vector<size_t> found;
        size_t quiet = 400;

        for (size_t i = 0; i < x.size(); ++i)
        {
            if (std::abs (x[i]) > 0.01f)
            {
                if (quiet >= 400)
                    found.push_back (i);
                quiet = 0;
            }
            else
            {
                ++quiet;
            }
        }

        return found;
    };

    auto runFor = [&] (int blocks)
    {
        setParam (processor, id::run, 1.0f);
        const auto hits = onsets (render (blocks)).size();
        setParam (processor, id::run, 0.0f);
        render (40);
        return (int) hits;
    };

    // 120 BPM sixteenths = 6000 samples per step; 93 blocks = 8 steps (not yet the 9th)
    const auto plain = runFor (93);

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
        setParam (processor, id::stepRatchet (i), 4.0f);

    const auto ratcheted = runFor (93);
    check (plain == 8 && ratcheted == 32, "ratchet 4 plays four hits per step");

    setParam (processor, id::run, 1.0f);
    const auto spacing = onsets (render (12));
    setParam (processor, id::run, 0.0f);
    render (40);
    // the onset detector is a few samples late on a hit that starts at a low point of the wave
    check (spacing.size() >= 4 && std::abs ((double) (spacing[2] - spacing[0]) - 3000.0) <= 12.0
               && std::abs ((double) (spacing[3] - spacing[1]) - 3000.0) <= 12.0,
           "ratchet hits are spread evenly over the step");

    check (onsets (render (12, noteOn (PS8AudioProcessor::firstStepNote))).size() == 4, "a MIDI note plays the step's ratchets too");

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
    {
        setParam (processor, id::stepRatchet (i), 1.0f);
        setParam (processor, id::stepChance (i), 0.0f);
    }

    check (runFor (93) == 0, "chance 0 % never plays a clocked step");
    check (onsets (render (12, noteOn (PS8AudioProcessor::firstStepNote))).size() == 1, "a MIDI note ignores the chance");

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
        setParam (processor, id::stepChance (i), 0.5f);

    const auto half = runFor (1125);   // 96 steps (1125 blocks = 95.9 steps)
    std::printf ("      chance 50 %%: %d of 96 steps\n", half);
    check (half > 28 && half < 68, "chance 50 % plays about half of the steps");

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
        setParam (processor, id::stepChance (i), 1.0f);

    // swing 75 %: every second step moves half a step later (6000 samples per step)
    setParam (processor, id::swing, 0.75f);
    setParam (processor, id::run, 1.0f);
    const auto swung = onsets (render (60));
    setParam (processor, id::run, 0.0f);
    render (40);
    setParam (processor, id::swing, 0.5f);

    auto gap = [&swung] (size_t a) { return (double) swung[a + 1] - (double) swung[a]; };
    check (swung.size() >= 4 && std::abs (gap (0) - 9000.0) <= 12.0 && std::abs (gap (1) - 3000.0) <= 12.0
               && std::abs (gap (2) - 9000.0) <= 12.0,
           "swing 75 % delays every second step by half a step");
}

//==============================================================================
// Like clap-validator's param-conversions: value -> text -> value -> text must give the same text.
static void testTextConversions()
{
    PS8AudioProcessor processor;
    int checked = 0;
    juce::StringArray broken;

    for (auto* param : processor.getParameters())
    {
        for (int i = 0; i <= 100; ++i)
        {
            const auto text = param->getText ((float) i / 100.0f, 64);
            const auto back = param->getText (param->getValueForText (text), 64);
            ++checked;

            if (back != text)
            {
                broken.add (param->getName (64) + ": " + text + " -> " + back);
                break;
            }
        }
    }

    for (const auto& b : broken)
        std::printf ("      %s\n", b.toRawUTF8());

    check (checked > 0 && broken.isEmpty(), "every parameter turns its text back into the same value");
}

//==============================================================================
// Renders four bars of the init pattern (internal clock, 120 BPM) to a 16-bit WAV file.
static int renderDemo (const char* path)
{
    namespace id = ps8::params::id;
    constexpr int sampleRate = 48000;

    PS8AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, sampleRate, 512);
    setParam (processor, id::hostClock, 0.0f);
    setParam (processor, id::run, 1.0f);
    processor.prepareToPlay (sampleRate, 512);

    std::vector<float> out;
    juce::AudioBuffer<float> buffer (2, 512);

    for (int i = 0; i < sampleRate * 8 / 512; ++i)
    {
        juce::MidiBuffer none;
        buffer.clear();
        processor.processBlock (buffer, none);
        out.insert (out.end(), buffer.getReadPointer (0), buffer.getReadPointer (0) + 512);
    }

    juce::MemoryOutputStream wav;
    const auto dataSize = (uint32_t) out.size() * 2;
    wav.write ("RIFF", 4); wav.writeInt ((int) (36 + dataSize)); wav.write ("WAVE", 4);
    wav.write ("fmt ", 4); wav.writeInt (16); wav.writeShort (1); wav.writeShort (1);
    wav.writeInt (sampleRate); wav.writeInt (sampleRate * 2); wav.writeShort (2); wav.writeShort (16);
    wav.write ("data", 4); wav.writeInt ((int) dataSize);

    for (auto v : out)
        wav.writeShort ((short) juce::jlimit (-32767, 32767, (int) std::lround (v * 32767.0f)));

    const auto ok = juce::File (juce::String::fromUTF8 (path)).replaceWithData (wav.getData(), wav.getDataSize());
    std::printf ("%s %s\n", ok ? "wrote" : "could not write", path);
    return ok ? 0 : 1;
}

// Renders the editor while the service hatch opens (frames at 0.15 s steps) to check the animation.
static int hatchFrames (const char* folder)
{
    PS8AudioProcessor processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (FrontPanel::designWidth, FrontPanel::designHeight);
    setParam (processor, ps8::params::id::serviceOpen, 1.0f);
    int frame = 0;

    for (int i = 0; i < 8; ++i)
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil (150);
        const auto image = editor->createComponentSnapshot (editor->getLocalBounds().withHeight (420), true, 1.0f);
        juce::FileOutputStream stream (juce::File (juce::String::fromUTF8 (folder)).getChildFile ("hatch-" + juce::String (frame++) + ".png"));
        stream.setPosition (0);
        stream.truncate();
        juce::PNGImageFormat().writeImageToStream (image, stream);
    }

    return 0;
}

// Renders the editor to a PNG file (optionally with a few cables patched and/or the service hatch open).
static int snapshot (const char* path, bool patched, bool service)
{
    PS8AudioProcessor processor;

    if (service)
        setParam (processor, ps8::params::id::serviceOpen, 1.0f);

    if (patched)
    {
        using namespace ps8::patch;
        processor.connect (vcaEgOut, tempoIn);
        processor.connect (vco2Out, vcfModIn);
        processor.connect (pitchOut, vco1CvIn);
        processor.connect (trigOut, resetIn);
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    editor->setSize (FrontPanel::designWidth * 2, FrontPanel::designHeight * 2);

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.0f);
    juce::FileOutputStream stream (juce::File (juce::String::fromUTF8 (path)));
    stream.setPosition (0);
    stream.truncate();
    const auto ok = stream.openedOk() && juce::PNGImageFormat().writeImageToStream (image, stream);
    std::printf ("%s %s\n", ok ? "wrote" : "could not write", path);
    return ok ? 0 : 1;
}

// Renders the app icon to a PNG file (the source of design/icon.png).
static int renderIcon (const char* path, int size)
{
    juce::Image image (juce::Image::ARGB, size, size, true);
    {
        juce::Graphics g (image);
        ps8::ui::drawAppIcon (g, image.getBounds().toFloat());
    }

    juce::File file (juce::String::fromUTF8 (path));
    file.deleteFile();
    juce::FileOutputStream stream (file);
    const auto ok = stream.openedOk() && juce::PNGImageFormat().writeImageToStream (image, stream);
    std::printf ("%s %s\n", ok ? "wrote" : "could not write", path);
    return ok ? 0 : 1;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    // presets and settings go to a scratch folder, never to the user's real one
    const auto scratch = juce::File::getSpecialLocation (juce::File::tempDirectory)
                             .getChildFile ("PS8Tests-" + juce::String::toHexString (juce::Random::getSystemRandom().nextInt64()));
    ps8::setAppDataFolderOverride (scratch);
    struct CleanUp { juce::File folder; ~CleanUp() { folder.deleteRecursively(); } } cleanUp { scratch };

    if (argc == 3 && std::string_view (argv[1]) == "--render")
        return renderDemo (argv[2]);

    if (argc == 3 && std::string_view (argv[1]) == "--hatch-frames")
        return hatchFrames (argv[2]);

    if (argc >= 3 && std::string_view (argv[1]) == "--snapshot")
    {
        auto has = [argc, argv] (std::string_view option)
        {
            for (int i = 3; i < argc; ++i)
                if (option == argv[i])
                    return true;
            return false;
        };

        return snapshot (argv[2], has ("patched"), has ("service"));
    }

    if (argc == 3 && std::string_view (argv[1]) == "--icon")
        return renderIcon (argv[2], 1024);

    if (argc == 2 && std::string_view (argv[1]) == "--bench")
    {
        PS8AudioProcessor processor;
        processor.setPlayConfigDetails (0, 2, 48000.0, 512);
        setParam (processor, ps8::params::id::hostClock, 0.0f);
        setParam (processor, ps8::params::id::run, 1.0f);
        processor.prepareToPlay (48000.0, 512);
        juce::AudioBuffer<float> buffer (2, 512);
        const auto blocks = 48000 * 20 / 512;
        const auto start = juce::Time::getMillisecondCounterHiRes();

        for (int i = 0; i < blocks; ++i)
        {
            juce::MidiBuffer midi;
            processor.processBlock (buffer, midi);
        }

        const auto ms = juce::Time::getMillisecondCounterHiRes() - start;
        std::printf ("20 s audio in %.0f ms (%.2f %% of one core), latency %d samples\n",
                     ms, ms / 200.0, processor.getLatencySamples());
        return 0;
    }

    testEngine();
    testSequencer();
    testProcessor();
    testHostStart();
    testPatch();
    testPatchbayMouse();
    testPresets();
    testPatterns();
    testDoubleClick();
    testRatchetAndChance();
    testTextConversions();

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
