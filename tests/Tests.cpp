// Offline tests for the PS-8 engine, sequencer and processor.

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
// Renders four bars of the init pattern (internal clock, 120 BPM) to a 16-bit WAV file.
static int renderDemo (const char* path)
{
    namespace id = ps8::params::id;
    constexpr int sampleRate = 48000;

    PS8AudioProcessor processor;
    processor.setPlayConfigDetails (0, 2, sampleRate, 512);
    setParam (processor, id::hostClock, 0.0f);
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

// Renders the editor to a PNG file.
static int snapshot (const char* path)
{
    PS8AudioProcessor processor;
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

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    if (argc == 3 && std::string_view (argv[1]) == "--render")
        return renderDemo (argv[2]);

    if (argc == 3 && std::string_view (argv[1]) == "--snapshot")
        return snapshot (argv[2]);

    if (argc == 2 && std::string_view (argv[1]) == "--bench")
    {
        PS8AudioProcessor processor;
        processor.setPlayConfigDetails (0, 2, 48000.0, 512);
        setParam (processor, ps8::params::id::hostClock, 0.0f);
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
    testDoubleClick();

    std::printf ("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASSED" : "FAILED", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
