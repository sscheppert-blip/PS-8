#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
    namespace id = ps8::params::id;
}

PS8AudioProcessor::PS8AudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                                       .withInput ("Ext In", juce::AudioChannelSet::stereo(), false)),
      state (*this, nullptr, "PS8", ps8::params::createLayout (wrapperType == wrapperType_Standalone))
{
    auto raw = [this] (const juce::String& paramId)
    {
        auto* value = state.getRawParameterValue (paramId);
        jassert (value != nullptr);
        return value;
    };

    refs.vco1Freq = raw (id::vco1Freq);     refs.vco1Eg = raw (id::vco1Eg);
    refs.vco1Square = raw (id::vco1Square); refs.vco1Level = raw (id::vco1Level);
    refs.vco2Freq = raw (id::vco2Freq);     refs.vco2Eg = raw (id::vco2Eg);
    refs.vco2Square = raw (id::vco2Square); refs.vco2Level = raw (id::vco2Level);
    refs.fmAmount = raw (id::fmAmount);     refs.fmOn = raw (id::fmOn);         refs.hardSync = raw (id::hardSync);
    refs.noiseLevel = raw (id::noiseLevel); refs.noisePink = raw (id::noisePink); refs.noiseVcf = raw (id::noiseVcf);
    refs.cutoff = raw (id::cutoff);         refs.resonance = raw (id::resonance);
    refs.vcfEg = raw (id::vcfEg);           refs.highPass = raw (id::highPass);
    refs.pitchDecay = raw (id::pitchDecay); refs.vcfDecay = raw (id::vcfDecay); refs.vcaDecay = raw (id::vcaDecay);
    refs.vcaSlow = raw (id::vcaSlow);       refs.drive = raw (id::drive);       refs.volume = raw (id::volume);
    refs.tempo = raw (id::tempo);           refs.run = raw (id::run);           refs.hostClock = raw (id::hostClock);
    refs.rate = raw (id::rate);             refs.pitchTarget = raw (id::pitchTarget);
    refs.length = raw (id::length);         refs.scale = raw (id::scale);       refs.glide = raw (id::glide);
    refs.root = raw (id::root);             refs.swing = raw (id::swing);
    refs.lfoRate = raw (id::lfoRate);       refs.lfoDivision = raw (id::lfoDivision);
    refs.lfoSync = raw (id::lfoSync);       refs.lfoShape = raw (id::lfoShape);
    refs.lfoDepth = raw (id::lfoDepth);     refs.lfoTarget = raw (id::lfoTarget);

    refs.analog = raw (id::analog);
    unitSeed = (uint32_t) juce::Random::getSystemRandom().nextInt();
    presets = std::make_unique<ps8::Presets> (*this);
    patterns = std::make_unique<ps8::Patterns> (state);

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
    {
        refs.pitch[(size_t) i] = raw (id::stepPitch (i));
        refs.velocity[(size_t) i] = raw (id::stepVelocity (i));
        refs.on[(size_t) i] = raw (id::stepOn (i));
        refs.ratchet[(size_t) i] = raw (id::stepRatchet (i));
        refs.chance[(size_t) i] = raw (id::stepChance (i));
    }
}

void PS8AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // The oscillators, FM and the ladder run oversampled: 4x up to 48 kHz, 2x up to 96 kHz.
    const auto factorExponent = sampleRate <= 50000.0 ? 2 : (sampleRate <= 100000.0 ? 1 : 0);
    oversamplingFactor = 1 << factorExponent;
    maxChunk = juce::jmax (samplesPerBlock, 32);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        1, (size_t) factorExponent, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampler->initProcessing ((size_t) maxChunk);
    setLatencySamples (juce::roundToInt (oversampler->getLatencyInSamples()));

    zeroBuffer.setSize (1, maxChunk);
    zeroBuffer.clear();
    oversampledBuffer.setSize (1, maxChunk * oversamplingFactor);
    extBuffer.setSize (1, maxChunk);

    engine.setSettings (readSettings ((double) refs.tempo->load()));
    appliedUnitSeed = unitSeed.load();
    engine.setTolerances (ps8::dsp::Tolerances::fromSeed (appliedUnitSeed));
    engine.prepare (sampleRate * oversamplingFactor);
    sequencer.prepare (sampleRate);
    hostSampleRate = sampleRate;
    ratchet = {};
    swingPending = {};
    samplesSinceClock = -1.0;
    hostWasRunning = false;
    runGate = clockEdge = resetEdge = {};
    loadMeasurer.reset (sampleRate, samplesPerBlock);
}

bool PS8AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in = layouts.getMainInputChannelSet();
    auto monoOrStereo = [] (const juce::AudioChannelSet& set)
    {
        return set == juce::AudioChannelSet::mono() || set == juce::AudioChannelSet::stereo();
    };

    return monoOrStereo (out) && (in.isDisabled() || monoOrStereo (in));
}

//==============================================================================
ps8::dsp::Settings PS8AudioProcessor::readSettings (double bpm) const noexcept
{
    auto on = [] (const std::atomic<float>* p) { return p->load() > 0.5f; };

    ps8::dsp::Settings s;
    s.vco1Freq = refs.vco1Freq->load();     s.vco1Eg = refs.vco1Eg->load();
    s.vco1Square = on (refs.vco1Square);    s.vco1Level = refs.vco1Level->load();
    s.vco2Freq = refs.vco2Freq->load();     s.vco2Eg = refs.vco2Eg->load();
    s.vco2Square = on (refs.vco2Square);    s.vco2Level = refs.vco2Level->load();
    s.fmAmount = refs.fmAmount->load();     s.fmOn = on (refs.fmOn);            s.hardSync = on (refs.hardSync);
    s.noiseLevel = refs.noiseLevel->load(); s.noisePink = on (refs.noisePink);  s.noiseVcf = refs.noiseVcf->load();
    s.cutoff = refs.cutoff->load();         s.resonance = refs.resonance->load();
    s.vcfEg = refs.vcfEg->load();           s.highPass = on (refs.highPass);
    s.pitchDecay = refs.pitchDecay->load(); s.vcfDecay = refs.vcfDecay->load(); s.vcaDecay = refs.vcaDecay->load();
    s.analog = refs.analog->load();
    s.vcaSlowAttack = on (refs.vcaSlow);    s.drive = refs.drive->load();       s.volume = refs.volume->load();
    s.pitchTarget = (ps8::dsp::PitchTarget) juce::jlimit (0, 2, juce::roundToInt (refs.pitchTarget->load()));
    s.glide = refs.glide->load();
    s.lfoDepth = refs.lfoDepth->load();
    s.lfoShape = (ps8::dsp::LfoShape) juce::jlimit (0, 4, juce::roundToInt (refs.lfoShape->load()));
    s.lfoTarget = (ps8::dsp::LfoTarget) juce::jlimit (0, 4, juce::roundToInt (refs.lfoTarget->load()));
    s.lfoHz = on (refs.lfoSync) ? (float) (bpm / 60.0 / lfoBeats())
                                : ps8::dsp::lfoHz (refs.lfoRate->load());
    return s;
}

double PS8AudioProcessor::lfoBeats() const noexcept
{
    const auto index = juce::jlimit (0, (int) std::size (ps8::params::lfoDivisionBeats) - 1,
                                     juce::roundToInt (refs.lfoDivision->load()));
    return ps8::params::lfoDivisionBeats[(size_t) index];
}

void PS8AudioProcessor::playStep (int step, float velocityScale, bool clocked) noexcept
{
    const auto i = (size_t) step;
    display.step = step;
    ratchet.remaining = 0;   // a new step cuts the rest of the previous one

    if (refs.on[i]->load() <= 0.5f)
        return;

    if (clocked && random.nextFloat() >= refs.chance[i]->load())
        return;

    const auto scale = (ps8::seq::Scale) juce::jlimit (0, 4, juce::roundToInt (refs.scale->load()));
    const auto root = juce::jlimit (0, 11, juce::roundToInt (refs.root->load()));
    const auto semitones = ps8::seq::stepPitch (ps8::dsp::stepSemitones (refs.pitch[i]->load()), scale, root);
    const auto velocity = refs.velocity[i]->load() * velocityScale;
    engine.trigger (velocity, semitones);

    if (const auto hits = juce::jlimit (1, ps8::params::maxRatchet, juce::roundToInt (refs.ratchet[i]->load())); hits > 1)
    {
        ratchet.remaining = hits - 1;
        ratchet.interval = ratchet.countdown = juce::jmax (1.0, stepSamples / hits);
        ratchet.velocity = velocity;
        ratchet.semitones = semitones;
        ratchet.clocked = clocked;
    }
}

void PS8AudioProcessor::clockStep (int step, bool offbeat) noexcept
{
    // a pending offbeat always plays before the next step
    if (swingPending.step >= 0)
        playStep (std::exchange (swingPending.step, -1), 1.0f, true);

    // swing 50 % = straight, 75 % = the offbeat sits halfway into its step
    const auto delay = (refs.swing->load() - 0.5) * 2.0 * stepSamples;

    if (offbeat && delay >= 1.0)
        swingPending = { step, delay };
    else
        playStep (step, 1.0f, true);
}

void PS8AudioProcessor::playRatchets() noexcept
{
    if (ratchet.remaining > 0 && --ratchet.countdown <= 0.0)
    {
        engine.trigger (ratchet.velocity, ratchet.semitones);
        ratchet.countdown += ratchet.interval;
        --ratchet.remaining;
    }
}

void PS8AudioProcessor::handleMidi (const juce::MidiMessage& message) noexcept
{
    if (! message.isNoteOn())
        return;

    const auto note = message.getNoteNumber();
    const auto velocity = message.getFloatVelocity();
    ++display.midiNotes;

    if (note >= firstStepNote && note < firstStepNote + ps8::seq::maxSteps)
    {
        sequencer.setCurrent (note - firstStepNote);
        playStep (note - firstStepNote, velocity);
    }
    else if (note == advanceNote)
    {
        playStep (sequencer.advance(), velocity);
    }
    else if (note == triggerNote)
    {
        playStep (juce::jmax (0, sequencer.current()), velocity);
    }
    else if (note == resetNote)
    {
        sequencer.reset();
    }
}

bool PS8AudioProcessor::readExtInput (juce::AudioBuffer<float>& buffer, int numSamples)
{
    auto* bus = getBus (true, 0);

    if (bus == nullptr || ! bus->isEnabled() || bus->getNumberOfChannels() == 0)
        return false;

    // the input shares its channels with the output, so copy it before rendering
    const auto input = getBusBuffer (buffer, true, 0);
    const auto channels = input.getNumChannels();

    if (extBuffer.getNumSamples() < numSamples)
        extBuffer.setSize (1, numSamples, false, false, true);

    extBuffer.copyFrom (0, 0, input, 0, 0, numSamples);

    for (int c = 1; c < channels; ++c)
        extBuffer.addFrom (0, 0, input, c, 0, numSamples);

    if (channels > 1)
        extBuffer.applyGain (0, 0, numSamples, 1.0f / (float) channels);

    return true;
}

void PS8AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    namespace patch = ps8::patch;
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    juce::AudioProcessLoadMeasurer::ScopedTimer timer (loadMeasurer, numSamples);

    juce::Optional<double> hostBpm, hostPpq;
    bool hostPlaying = false;

    if (auto* head = getPlayHead())
    {
        if (const auto position = head->getPosition())
        {
            hostBpm = position->getBpm();
            hostPpq = position->getPpqPosition();
            hostPlaying = position->getIsPlaying();
        }
    }

    if (oversampledBuffer.getNumSamples() < numSamples * oversamplingFactor)
        oversampledBuffer.setSize (1, numSamples * oversamplingFactor, false, false, true);

    const auto hasExt = readExtInput (buffer, numSamples);
    engine.setExtConnected (hasExt);

    // patchbay
    {
        patch::Masks masks;

        for (size_t i = 0; i < masks.size(); ++i)
            masks[i] = patchMasks[i].load (std::memory_order_relaxed);

        engine.setPatch (masks);
    }

    const auto clockPatched = engine.isPatched (patch::advClockIn);
    const auto runPatched = engine.isPatched (patch::runIn);
    const auto resetPatched = engine.isPatched (patch::resetIn);
    const auto tempoPatched = engine.isPatched (patch::tempoIn);

    if (! clockPatched) { clockEdge = {}; samplesSinceClock = -1.0; }
    if (! runPatched)   runGate = {};
    if (! resetPatched) resetEdge = {};

    // sequencer ticks of this block (host clock)
    constexpr int maxTicks = 64;
    std::array<ps8::seq::Tick, maxTicks> ticks;
    int numTicks = 0;

    auto runSwitch = refs.run->load() > 0.5f;
    const auto rate = (ps8::seq::Rate) juce::jlimit (0, 5, juce::roundToInt (refs.rate->load()));
    // HOST without a DAW clock (e.g. the Standalone app) waits instead of running on its own
    const auto hostClockOn = refs.hostClock->load() > 0.5f;
    const auto followHost = hostClockOn && hostBpm.hasValue() && hostPpq.hasValue();
    const auto waitingForHost = hostClockOn && ! followHost;
    const auto bpm = followHost ? juce::jlimit (20.0, 999.0, *hostBpm) : (double) refs.tempo->load();

    sequencer.setLength (juce::roundToInt (refs.length->load()));

    if (followHost)
    {
        // a cable in ADV/CLOCK replaces the clock
        const auto hostRunning = runSwitch && hostPlaying && ! clockPatched;

        if (hostRunning)
            numTicks = sequencer.runHost (*hostPpq, bpm, numSamples, rate, ! hostWasRunning, ticks.data(), maxTicks);

        hostWasRunning = hostRunning;
    }
    else
    {
        hostWasRunning = false;
    }

    if (const auto seed = unitSeed.load(); seed != appliedUnitSeed)
    {
        appliedUnitSeed = seed;
        engine.setTolerances (ps8::dsp::Tolerances::fromSeed (seed));
    }

    display.bpm = (float) bpm;
    display.hostLocked = followHost && hostPlaying;
    engine.setSettings (readSettings (bpm));

    // a synced LFO follows the host's beat position
    if (refs.lfoSync->load() > 0.5f && followHost && hostPlaying)
    {
        const auto cycles = *hostPpq / lfoBeats();
        engine.setLfoPhase ((float) (cycles - std::floor (cycles)));
    }

    // front panel buttons act at the start of the block
    if (scramRequest.exchange (false))
    {
        engine.panic();
        ratchet = {};
        swingPending = {};
        sequencer.reset();
        sequencer.stop();
        numTicks = 0;
        runSwitch = false;
    }

    if (resetRequest.exchange (false))
        sequencer.reset();

    if (advanceRequest.exchange (false))
        playStep (sequencer.advance(), 1.0f);

    if (triggerRequest.exchange (false))
        playStep (juce::jmax (0, sequencer.current()), 1.0f);

    // render one host sample at a time, so cables into the sequencer act on the next sample
    engine.beginBlock();
    auto* oversampled = oversampledBuffer.getWritePointer (0);
    const auto* ext = hasExt ? extBuffer.getReadPointer (0) : nullptr;
    auto event = midi.cbegin();
    int nextTick = 0;
    bool running = false;
    auto shownBpm = bpm;
    const auto gridSamples = hostSampleRate * 60.0 / ps8::seq::stepsPerQuarter (rate);

    if (! clockPatched)
        stepSamples = gridSamples / bpm;

    for (int i = 0; i < numSamples; ++i)
    {
        if (resetPatched && resetEdge.process (engine.input (patch::resetIn)))
            sequencer.reset();

        if (runPatched)
            runGate.process (engine.input (patch::runIn));

        const auto clockEdgeNow = clockPatched && clockEdge.process (engine.input (patch::advClockIn));
        running = runSwitch && (! runPatched || runGate.high) && (! followHost || hostPlaying) && ! waitingForHost;

        if (clockPatched)
        {
            // an external clock sets the step length by its own period
            if (clockEdgeNow)
            {
                if (samplesSinceClock > 0.0)
                    stepSamples = samplesSinceClock;

                samplesSinceClock = 0.0;
            }

            if (samplesSinceClock >= 0.0)
                samplesSinceClock += 1.0;

            if (clockEdgeNow && running)
                playStep (sequencer.advance(), 1.0f, true);
        }
        else if (followHost)
        {
            for (; nextTick < numTicks && ticks[(size_t) nextTick].position <= i; ++nextTick)
                if (running)
                    clockStep (ticks[(size_t) nextTick].step, ticks[(size_t) nextTick].offbeat);
        }
        else if (running)
        {
            // TEMPO: 1 V doubles the tempo
            const auto sampleBpm = tempoPatched ? juce::jlimit (1.0, 2000.0, bpm * std::exp2 (5.0 * engine.input (patch::tempoIn)))
                                                : bpm;
            shownBpm = sampleBpm;
            stepSamples = gridSamples / sampleBpm;

            if (const auto step = sequencer.clockInternal (sampleBpm, rate); step >= 0)
                clockStep (step, sequencer.isOffbeat());
        }
        else
        {
            sequencer.stop();
        }

        // stopping the sequencer also stops the rest of a clocked ratchet and a waiting offbeat
        if (! running && ratchet.clocked)
            ratchet.remaining = 0;

        if (! running)
            swingPending = {};
        else if (swingPending.step >= 0 && --swingPending.countdown <= 0.0)
            playStep (std::exchange (swingPending.step, -1), 1.0f, true);

        playRatchets();

        for (; event != midi.cend() && juce::jlimit (0, juce::jmax (0, numSamples - 1), (*event).samplePosition) <= i; ++event)
            handleMidi ((*event).getMessage());

        if (ext != nullptr)
            engine.setExtSample (ext[i]);

        engine.renderSamples (oversampled + i * oversamplingFactor, oversamplingFactor);
    }

    midi.clear();
    display.running = running;

    if (tempoPatched && ! followHost)
        display.bpm = (float) juce::jmin (999.0, shownBpm);

    // back to the host rate
    auto output = getBusBuffer (buffer, false, 0);
    auto* mono = output.getWritePointer (0);

    for (int start = 0; start < numSamples; start += maxChunk)
    {
        const auto length = juce::jmin (maxChunk, numSamples - start);
        const auto zeros = juce::dsp::AudioBlock<float> (zeroBuffer).getSubBlock (0, (size_t) length);
        auto up = oversampler->processSamplesUp (zeros);
        juce::FloatVectorOperations::copy (up.getChannelPointer (0), oversampled + start * oversamplingFactor,
                                           length * oversamplingFactor);

        float* channels[] = { mono + start };
        juce::dsp::AudioBlock<float> down (channels, 1, (size_t) length);
        oversampler->processSamplesDown (down);
    }

    const auto blockPeak = output.getMagnitude (0, 0, numSamples);

    if (blockPeak > display.peak.load())
        display.peak = blockPeak;

    display.lfoHigh = refs.lfoDepth->load() > 0.0f && engine.getLfoPhase() < 0.5f;

    for (int c = 1; c < output.getNumChannels(); ++c)
        output.copyFrom (c, 0, output, 0, 0, numSamples);

    for (int c = output.getNumChannels(); c < buffer.getNumChannels(); ++c)
        buffer.clear (c, 0, numSamples);
}

//==============================================================================
bool PS8AudioProcessor::isConnected (ps8::patch::Output o, ps8::patch::Input in) const noexcept
{
    return (patchMasks[(size_t) in].load() >> o) & 1u;
}

void PS8AudioProcessor::connect (ps8::patch::Output o, ps8::patch::Input in) noexcept
{
    patchMasks[(size_t) in].fetch_or ((ps8::patch::Mask) (1u << o));
    ++patchVersion;
}

void PS8AudioProcessor::disconnect (ps8::patch::Output o, ps8::patch::Input in) noexcept
{
    patchMasks[(size_t) in].fetch_and ((ps8::patch::Mask) ~(1u << o));
    ++patchVersion;
}

void PS8AudioProcessor::clearPatch() noexcept
{
    setPatch ({});
}

ps8::patch::Masks PS8AudioProcessor::getPatch() const noexcept
{
    ps8::patch::Masks masks;

    for (size_t i = 0; i < masks.size(); ++i)
        masks[i] = patchMasks[i].load();

    return masks;
}

void PS8AudioProcessor::setPatch (const ps8::patch::Masks& masks) noexcept
{
    for (size_t i = 0; i < masks.size(); ++i)
        patchMasks[i] = masks[i];

    ++patchVersion;
}

// "vcaEg>tempo vco2>vcfMod"
juce::String PS8AudioProcessor::patchToString (const ps8::patch::Masks& masks)
{
    juce::StringArray cables;

    for (int in = 0; in < ps8::patch::numInputs; ++in)
        for (int o = 0; o < ps8::patch::numOutputs; ++o)
            if ((masks[(size_t) in] >> o) & 1u)
                cables.add (juce::String (ps8::patch::outputIds[o]) + ">" + ps8::patch::inputIds[in]);

    return cables.joinIntoString (" ");
}

ps8::patch::Masks PS8AudioProcessor::patchFromString (const juce::String& text)
{
    ps8::patch::Masks masks {};

    auto indexOf = [] (const auto& ids, int count, const juce::String& name)
    {
        for (int i = 0; i < count; ++i)
            if (name == ids[i])
                return i;
        return -1;
    };

    for (const auto& cable : juce::StringArray::fromTokens (text, " ", {}))
    {
        const auto o = indexOf (ps8::patch::outputIds, ps8::patch::numOutputs, cable.upToFirstOccurrenceOf (">", false, false));
        const auto in = indexOf (ps8::patch::inputIds, ps8::patch::numInputs, cable.fromFirstOccurrenceOf (">", false, false));

        if (o >= 0 && in >= 0)
            masks[(size_t) in] |= (ps8::patch::Mask) (1u << o);
    }

    return masks;
}

//==============================================================================
void PS8AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto tree = state.copyState();
    tree.setProperty (patchProperty, patchToString (getPatch()), nullptr);
    tree.setProperty (presetProperty, presets->getStateName(), nullptr);
    tree.setProperty (unitProperty, (juce::int64) unitSeed.load(), nullptr);
    tree.appendChild (patterns->toTree(), nullptr);

    if (const auto xml = tree.createXml())
        copyXmlToBinary (*xml, destData);
}

void PS8AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // The standalone app restores its last session before it opens the window; PS-8 starts with Init
    // instead. (Its "Load state" menu still works later.)
    if (wrapperType == wrapperType_Standalone && ! editorCreated)
        return;

    if (const auto xml = getXmlFromBinary (data, sizeInBytes))
    {
        if (xml->hasTagName (state.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            setPatch (patchFromString (tree.getProperty (patchProperty).toString()));

            const auto preset = tree.getProperty (presetProperty).toString();
            const auto isUser = preset.startsWith ("user:");
            presets->setStateName (isUser ? preset.substring (5) : preset, ! isUser);

            if (tree.hasProperty (unitProperty))
                unitSeed = (uint32_t) (juce::int64) tree.getProperty (unitProperty);

            tree.removeProperty (patchProperty, nullptr);
            tree.removeProperty (presetProperty, nullptr);
            tree.removeProperty (unitProperty, nullptr);

            const auto savedPatterns = tree.getChildWithName ("PATTERNS");
            tree.removeChild (savedPatterns, nullptr);
            state.replaceState (tree);
            patterns->fromTree (savedPatterns);
            ++stateVersion;

            // CLAP hosts expect a value rescan after a state load (the wrapper doesn't send one itself);
            // it maps "parameter info changed" to that rescan.
            if (is_clap)
                updateHostDisplay (ChangeDetails().withParameterInfoChanged (true));
        }
    }
}

juce::AudioProcessorEditor* PS8AudioProcessor::createEditor()
{
    editorCreated = true;
    return new PS8AudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PS8AudioProcessor();
}
