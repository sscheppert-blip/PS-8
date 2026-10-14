#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
    namespace id = ps8::params::id;
    namespace ui = ps8::ui;
    namespace colours = ps8::ui::colours;
    using Labels = ui::Toggle::Labels;

    // rows
    constexpr float auxTop = 380.0f;   // SEQUENCE / LFO / STATUS
    constexpr float seqTop = 528.0f;   // SEQUENCER / PATCH

    // sequencer columns (eight per page)
    constexpr int stepsPerPage = 8;
    constexpr float firstStepX = 260.0f, stepPitch = 78.0f;

    float stepX (int step) { return firstStepX + stepPitch * (float) (step % stepsPerPage); }

    struct JackInfo
    {
        const char* label;
        bool output;
    };

    constexpr JackInfo jacks[] =
    {
        { "TRIG", true },     { "VCA CV", false },  { "VELOCITY", true }, { "VCA DEC", false },
        { "EXT IN", false },  { "VCF DEC", false }, { "NOISE", false },   { "VCF MOD", false },
        { "VCO DEC", false }, { "VCO 1", true },    { "1>2 FM", false },  { "VCO 2", true },
        { "PITCH", true },    { "RUN", false },     { "ADV", false },     { "RESET", false },
    };

    juce::Point<float> jackPosition (int i)
    {
        return { 915.0f + 74.0f * (float) (i % 4), seqTop + 72.0f + 50.0f * (float) (i / 4) };
    }
}

FrontPanel::FrontPanel (PS8AudioProcessor& p) : processor (p)
{
    setSize (designWidth, designHeight);
    setOpaque (true);

    // top bar
    outputGauge = &make<ui::Gauge> ("OUTPUT", juce::StringArray { "-20", "0", "+3" });
    outputGauge->setBounds (376, 22, 148, 90);
    loadGauge = &make<ui::Gauge> ("LOAD", juce::StringArray { "0", "50", "100" });
    loadGauge->setBounds (536, 22, 148, 90);

    bpmNixie = &make<ui::Nixie> (3);
    bpmNixie->setTopLeftPosition (720, 50);
    stepNixie = &make<ui::Nixie> (2);
    stepNixie->setTopLeftPosition (906, 50);
    runLamp = &make<ui::Lamp> (juce::Colour (0xff5ff07a), juce::Colour (0xff1e4a26));
    runLamp->setBounds (848, 63, 24, 24);

    auto& scramButton = make<ui::PushButton> ("SCRAM", true);
    scramButton.place ({ 1100.0f, 68.0f }, 31.0f);
    scramButton.onPress = [this] { scram(); };

    // oscillators
    knob (id::vco1Freq, "FREQUENCY", 64, 196, 22);
    knob (id::vco1Eg, "EG AMOUNT", 144, 196, 18, true);
    toggle (id::vco1Square, 64, 300, { 1.0f, 0.0f }, { "SQR", "TRI", "WAVE" });
    knob (id::vco1Level, "LEVEL", 144, 300, 18);

    knob (id::vco2Freq, "FREQUENCY", 240, 196, 22);
    knob (id::vco2Eg, "EG AMOUNT", 320, 196, 18, true);
    toggle (id::vco2Square, 240, 300, { 1.0f, 0.0f }, { "SQR", "TRI", "WAVE" });
    knob (id::vco2Level, "LEVEL", 320, 300, 18);

    // modulation
    knob (id::fmAmount, "FM 1>2 AMOUNT", 442, 196, 22);
    toggle (id::fmOn, 404, 300, { 1.0f, 0.0f }, { "ON", "OFF", "FM" });
    toggle (id::hardSync, 468, 300, { 1.0f, 0.0f }, { "ON", "OFF", "HARD SYNC" });

    // noise
    knob (id::noiseLevel, "LEVEL", 585, 196, 20);
    toggle (id::noisePink, 553, 300, { 0.0f, 1.0f }, { "WHITE", "PINK", "COLOR" });
    knob (id::noiseVcf, "VCF MOD", 618, 300, 14);

    // filter
    knob (id::cutoff, "CUTOFF", 706, 196, 24);
    knob (id::resonance, "RESONANCE", 800, 196, 20);
    knob (id::vcfEg, "EG AMOUNT", 706, 300, 18, true);
    toggle (id::highPass, 800, 300, { 0.0f, 1.0f }, { "LOW", "HIGH", "MODE" });

    // decays
    knob (id::pitchDecay, "PITCH", 896, 196, 18);
    knob (id::vcfDecay, "FILTER", 964, 196, 18);
    knob (id::vcaDecay, "VCA", 1032, 196, 18);
    toggle (id::vcaSlow, 912, 300, { 0.0f, 1.0f }, { "FAST", "SLOW", "VCA ATTACK" });
    knob (id::drive, "DRIVE", 1016, 300, 18);

    // output
    knob (id::volume, "VOLUME", 1130, 200, 24);
    clipLamp = &make<ui::Lamp> (juce::Colour (0xffff4a35), juce::Colour (0xff4a1410));
    clipLamp->setBounds (1116, 282, 28, 28);

    // sequence extras
    const auto auxKnobY = auxTop + 60.0f;
    knob (id::rate, "RATE", 62, auxKnobY, 16, false, 6, true);
    knob (id::length, "LENGTH", 142, auxKnobY, 16, false, 16, true);
    knob (id::scale, "SCALE", 222, auxKnobY, 16, false, 5, true);
    knob (id::glide, "GLIDE", 302, auxKnobY, 16, false, 11, true);

    // LFO
    lfoRateKnob = &knob (id::lfoRate, "RATE", 418, auxKnobY, 16, false, 11, true);
    knob (id::lfoShape, "SHAPE", 498, auxKnobY, 16, false, 5, true);
    knob (id::lfoDepth, "DEPTH", 578, auxKnobY, 16, false, 11, true);
    knob (id::lfoTarget, "TARGET", 658, auxKnobY, 16, false, 5, true);
    toggle (id::lfoSync, 728, auxKnobY + 6.0f, { 0.0f, 1.0f }, { "FREE", "SYNC" });

    // status
    toggle (id::viewPage, 812, auxKnobY + 6.0f, { 0.0f, 1.0f }, { "1-8", "9-16", "STEPS" });
    hostLamp = &make<ui::Annunciator> ("HOST LOCK", juce::Colour (0xff8fe39b));
    midiLamp = &make<ui::Annunciator> ("MIDI IN", colours::amber);
    lfoLamp = &make<ui::Annunciator> ("LFO", colours::amber);
    pageLamp = &make<ui::Annunciator> ("STEPS 9-16", juce::Colour (0xffe8e2c8));
    hostLamp->setBounds (870, (int) auxTop + 40, 146, 36);
    midiLamp->setBounds (1024, (int) auxTop + 40, 146, 36);
    lfoLamp->setBounds (870, (int) auxTop + 84, 146, 36);
    pageLamp->setBounds (1024, (int) auxTop + 84, 146, 36);

    // sequencer
    knob (id::tempo, "TEMPO", 70, seqTop + 72.0f, 22);
    toggle (id::run, 44, seqTop + 168.0f, { 1.0f, 0.0f }, { "RUN", "STOP" });
    toggle (id::hostClock, 96, seqTop + 168.0f, { 0.0f, 1.0f }, { "INT", "HOST" });
    toggle (id::pitchTarget, 148, seqTop + 168.0f, { 0.0f, 1.0f, 2.0f }, { "1+2", "VCO 2", "PITCH TO", "OFF", true });

    auto& trig = make<ui::PushButton> ("TRIG");
    trig.place ({ 52.0f, seqTop + 240.0f }, 8.0f);
    trig.onPress = [this] { processor.requestTrigger(); };
    auto& advance = make<ui::PushButton> ("ADV");
    advance.place ({ 100.0f, seqTop + 240.0f }, 8.0f);
    advance.onPress = [this] { processor.requestAdvance(); };
    auto& reset = make<ui::PushButton> ("RESET");
    reset.place ({ 148.0f, seqTop + 240.0f }, 8.0f);
    reset.onPress = [this] { processor.requestReset(); };

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
    {
        auto& column = stepColumns[(size_t) i];
        column.pitch = &knob (id::stepPitch (i), {}, stepX (i), seqTop + 84.0f, 16, true);
        column.velocity = &knob (id::stepVelocity (i), {}, stepX (i), seqTop + 158.0f, 14);
        column.lamp = &make<ui::LampButton> (processor.getState(), id::stepOn (i));
        column.lamp->place ({ stepX (i), seqTop + 222.0f });
    }

    showPage (0);
    updateLfoRateKnob (false);
    startTimerHz (30);
}

ui::Knob& FrontPanel::knob (const juce::String& paramId, const juce::String& label, float x, float y, float radius,
                            bool bipolar, int ticks, bool showValue)
{
    auto& k = make<ui::Knob> (processor.getState(), paramId, label, radius, ticks, bipolar, showValue);
    k.place ({ x, y });
    return k;
}

ui::Toggle& FrontPanel::toggle (const juce::String& paramId, float x, float y, std::vector<float> positions,
                                Labels labels)
{
    auto& t = make<ui::Toggle> (processor.getState(), paramId, std::move (positions), std::move (labels));
    t.place ({ x, y });
    return t;
}

void FrontPanel::showPage (int page)
{
    shownPage = page;

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
    {
        const auto visible = i / stepsPerPage == page;
        auto& column = stepColumns[(size_t) i];
        column.pitch->setVisible (visible);
        column.velocity->setVisible (visible);
        column.lamp->setVisible (visible);
    }

    repaint();
}

void FrontPanel::updateLfoRateKnob (bool synced)
{
    lfoSynced = synced;
    lfoRateKnob->bind (processor.getState(), synced ? id::lfoDivision : id::lfoRate, synced ? 8 : 11);
}

void FrontPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::frame);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (6.0f, 6.0f, (float) designWidth - 12.0f, (float) designHeight - 12.0f, 7.0f);
    ui::drawHazard (g, { 6.0f, (float) designHeight - 36.0f, (float) designWidth - 12.0f, 30.0f });

    // name plate
    g.setColour (colours::ink);
    g.fillRoundedRectangle (20.0f, 18.0f, 330.0f, 100.0f, 4.0f);
    g.setColour (colours::plateText);
    g.drawRect (26.0f, 24.0f, 318.0f, 88.0f, 1.0f);
    g.setColour (colours::hazard);
    g.setFont (ui::stencilFont (46.0f));
    g.drawText ("PS-8", juce::Rectangle<float> (38.0f, 38.0f, 150.0f, 56.0f), juce::Justification::centredLeft, false);
    g.setColour (colours::plateText);
    g.setFont (ui::stencilFont (15.0f));
    g.drawText ("PERCUSSION", juce::Rectangle<float> (196.0f, 46.0f, 150.0f, 18.0f), juce::Justification::centredLeft, false);
    g.drawText ("SYNTHESIZER", juce::Rectangle<float> (196.0f, 66.0f, 150.0f, 18.0f), juce::Justification::centredLeft, false);
    ui::drawLabel (g, "UNIT 08 - CONTROL ROOM B", { 196.0f, 98.0f }, 10.0f, colours::dimText, juce::Justification::centredLeft);

    for (auto c : { juce::Point<float> (30, 28), { 340, 28 }, { 30, 108 }, { 340, 108 } })
        ui::drawScrew (g, c);

    // tempo / step display
    g.setColour (colours::ink);
    g.fillRoundedRectangle (706.0f, 22.0f, 290.0f, 92.0f, 4.0f);
    ui::drawLabel (g, "TEMPO BPM", { 770.0f, 38.0f }, 10.0f, colours::dimText);
    ui::drawLabel (g, "RUN", { 860.0f, 100.0f }, 10.0f, colours::dimText);
    ui::drawLabel (g, "STEP", { 938.0f, 38.0f }, 10.0f, colours::dimText);

    // modules
    ui::drawPlate (g, { 20.0f, 132.0f, 168.0f, 240.0f }, "OSCILLATOR 1");
    ui::drawPlate (g, { 196.0f, 132.0f, 168.0f, 240.0f }, "OSCILLATOR 2");
    ui::drawPlate (g, { 372.0f, 132.0f, 140.0f, 240.0f }, "MODULATION");
    ui::drawPlate (g, { 520.0f, 132.0f, 130.0f, 240.0f }, "NOISE");
    ui::drawPlate (g, { 658.0f, 132.0f, 190.0f, 240.0f }, "FILTER");
    ui::drawPlate (g, { 856.0f, 132.0f, 216.0f, 240.0f }, "DECAY");
    ui::drawPlate (g, { 1080.0f, 132.0f, 100.0f, 240.0f }, "OUTPUT");
    ui::drawLabel (g, "CLIP", { 1130.0f, 326.0f }, 12.0f);

    ui::drawPlate (g, { 20.0f, auxTop, 340.0f, 140.0f }, "SEQUENCE");
    ui::drawPlate (g, { 368.0f, auxTop, 392.0f, 140.0f }, "LFO");
    ui::drawPlate (g, { 768.0f, auxTop, 412.0f, 140.0f }, "STATUS");

    ui::drawPlate (g, { 20.0f, seqTop, 840.0f, 268.0f }, shownPage == 0 ? "SEQUENCER - STEPS 1-8" : "SEQUENCER - STEPS 9-16");

    for (int i = 0; i < stepsPerPage; ++i)
    {
        g.setColour (colours::ink);
        g.setFont (ui::stencilFont (14.0f));
        g.drawText (juce::String (shownPage * stepsPerPage + i + 1),
                    juce::Rectangle<float> (40.0f, 18.0f).withCentre ({ stepX (i), seqTop + 46.0f }),
                    juce::Justification::centred, false);
    }

    ui::drawLabel (g, "PITCH", { 230.0f, seqTop + 84.0f }, 11.0f, colours::ink, juce::Justification::centredRight);
    ui::drawLabel (g, "VELOCITY", { 230.0f, seqTop + 158.0f }, 11.0f, colours::ink, juce::Justification::centredRight);
    ui::drawLabel (g, "STEP ON", { 230.0f, seqTop + 222.0f }, 11.0f, colours::ink, juce::Justification::centredRight);

    ui::drawPlate (g, { 870.0f, seqTop, 310.0f, 268.0f }, "PATCH");

    for (int i = 0; i < (int) std::size (jacks); ++i)
        ui::drawJack (g, jackPosition (i), jacks[i].label, jacks[i].output);

    // the patchbay is not wired up yet
    {
        juce::Graphics::ScopedSaveState save (g);
        const auto centre = juce::Point<float> (1025.0f, seqTop + 154.0f);
        g.addTransform (juce::AffineTransform::rotation (-0.18f, centre.x, centre.y));
        const auto stamp = juce::Rectangle<float> (180.0f, 46.0f).withCentre (centre);
        g.setColour (colours::panel.withAlpha (0.85f));
        g.fillRoundedRectangle (stamp, 4.0f);
        g.setColour (colours::red.withAlpha (0.9f));
        g.drawRoundedRectangle (stamp.reduced (2.0f), 4.0f, 3.0f);
        g.setFont (ui::stencilFont (26.0f));
        g.drawText ("OFFLINE", stamp, juce::Justification::centred, false);
    }
}

void FrontPanel::scram()
{
    processor.requestScram();

    if (auto* run = processor.getState().getParameter (id::run))
    {
        run->beginChangeGesture();
        run->setValueNotifyingHost (0.0f);
        run->endChangeGesture();
    }

    alarmUntil = juce::Time::getMillisecondCounterHiRes() + 1500.0;
}

void FrontPanel::timerCallback()
{
    auto& state = processor.getState();
    auto& display = processor.getDisplay();
    const auto now = juce::Time::getMillisecondCounterHiRes();

    if (const auto page = state.getRawParameterValue (id::viewPage)->load() > 0.5f ? 1 : 0; page != shownPage)
        showPage (page);

    if (const auto synced = state.getRawParameterValue (id::lfoSync)->load() > 0.5f; synced != lfoSynced)
        updateLfoRateKnob (synced);

    const auto step = display.step.load();
    const auto length = juce::roundToInt (state.getRawParameterValue (id::length)->load());
    const auto alarm = now < alarmUntil;

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
        stepColumns[(size_t) i].lamp->setState (i == step, alarm, i < length);

    stepNixie->setText (step >= 0 ? juce::String (step + 1) : juce::String());
    bpmNixie->setText (juce::String (juce::roundToInt (display.bpm.load())));
    runLamp->setOn (display.running.load());

    // annunciators
    if (const auto notes = display.midiNotes.load(); notes != lastMidiNotes)
    {
        lastMidiNotes = notes;
        midiUntil = now + 120.0;
    }

    hostLamp->setOn (display.hostLocked.load());
    midiLamp->setOn (now < midiUntil);
    lfoLamp->setOn (display.lfoHigh.load());
    pageLamp->setOn (step >= stepsPerPage);

    // output needle: -20 dB at the left stop, 0 dB in the middle, +3 dB at the right stop
    const auto peak = display.peak.exchange (0.0f);
    const auto db = juce::Decibels::gainToDecibels (peak, -60.0f);
    const auto target = db <= 0.0f ? juce::jlimit (0.0f, 0.5f, 0.5f * (db + 20.0f) / 20.0f)
                                   : juce::jmin (1.0f, 0.5f + 0.5f * db / 3.0f);
    outputNeedle += (target - outputNeedle) * (target > outputNeedle ? 0.6f : 0.12f);
    outputGauge->setValue (outputNeedle);

    if (peak > 0.99f)
        clipUntil = now + 400.0;

    clipLamp->setOn (now < clipUntil);

    loadNeedle += ((float) processor.getCpuLoad() - loadNeedle) * 0.2f;
    loadGauge->setValue (loadNeedle);
}

//==============================================================================
PS8AudioProcessorEditor::PS8AudioProcessorEditor (PS8AudioProcessor& p)
    : AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) FrontPanel::designWidth / FrontPanel::designHeight);
    setResizeLimits (720, 505, 1800, 1263);
    setSize (1080, 758);
}

void PS8AudioProcessorEditor::resized()
{
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) FrontPanel::designWidth));
}
