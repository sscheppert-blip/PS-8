#include "PluginEditor.h"
#include "Parameters.h"

namespace
{
    namespace id = ps8::params::id;
    namespace ui = ps8::ui;
    namespace colours = ps8::ui::colours;
    using Labels = ui::Toggle::Labels;

    // rows
    const juce::Rectangle<int> hatchArea { 6, 120, 1188, 264 };   // module row: hood over the service board
    constexpr float seqTop = 388.0f;                              // SEQUENCER

    // service hatch timing in seconds
    constexpr double screwTime = 0.45, lidTime = 0.4;

    // sequencer columns (eight per page)
    constexpr int stepsPerPage = 8;
    constexpr float firstStepX = 290.0f, stepPitch = 118.0f;

    float stepX (int step) { return firstStepX + stepPitch * (float) (step % stepsPerPage); }
}

FrontPanel::FrontPanel (PS8AudioProcessor& p) : processor (p)
{
    setSize (designWidth, designHeight);
    setOpaque (true);

    // top bar
    outputGauge = &make<ui::Gauge> ("OUTPUT", juce::StringArray { "-20", "0", "+3" });
    outputGauge->setBounds (404, 22, 128, 90);
    loadGauge = &make<ui::Gauge> ("LOAD", juce::StringArray { "0", "50", "100" });
    loadGauge->setBounds (540, 22, 128, 90);

    bpmNixie = &make<ui::Nixie> (3);
    bpmNixie->setTopLeftPosition (694, 50);
    stepNixie = &make<ui::Nixie> (2);
    stepNixie->setTopLeftPosition (880, 50);
    runLamp = &make<ui::Lamp> (juce::Colour (0xff5ff07a), juce::Colour (0xff1e4a26));
    runLamp->setBounds (822, 63, 24, 24);

    programDisplay = &make<ui::ProgramDisplay> (processor);
    programDisplay->setTopLeftPosition (194, 27);

    auto& scramButton = make<ui::PushButton> ("SCRAM", true);
    scramButton.place ({ 1084.0f, 68.0f }, 31.0f);
    scramButton.onPress = [this] { scram(); };

    auto& key = make<ui::KeySwitch> (processor.getState(), id::serviceOpen);
    key.place ({ 1002.0f, 62.0f });

    // service hatch: the board lies under the hood that carries the module row
    board = &make<ui::ServiceBoard> (processor);
    board->setBounds (hatchArea);
    board->onClose = [this] { setService (false); };

    hood = &make<ui::Hood>();
    hood->setBounds (hatchArea);
    hood->painter = [this] (juce::Graphics& g) { paintModules (g); };
    hood->onUnscrew = [this] { setService (true); };
    auto* const h = hood;

    // oscillators
    knob (id::vco1Freq, "FREQUENCY", 64, 196, 22, false, 11, false, h);
    knob (id::vco1Eg, "EG AMOUNT", 144, 196, 18, true, 11, false, h);
    toggle (id::vco1Square, 64, 300, { 1.0f, 0.0f }, { "SQR", "TRI", "WAVE" }, h);
    knob (id::vco1Level, "LEVEL", 144, 300, 18, false, 11, false, h);

    knob (id::vco2Freq, "FREQUENCY", 240, 196, 22, false, 11, false, h);
    knob (id::vco2Eg, "EG AMOUNT", 320, 196, 18, true, 11, false, h);
    toggle (id::vco2Square, 240, 300, { 1.0f, 0.0f }, { "SQR", "TRI", "WAVE" }, h);
    knob (id::vco2Level, "LEVEL", 320, 300, 18, false, 11, false, h);

    // modulation
    knob (id::fmAmount, "FM 1>2 AMOUNT", 442, 196, 22, false, 11, false, h);
    toggle (id::fmOn, 404, 300, { 1.0f, 0.0f }, { "ON", "OFF", "FM" }, h);
    toggle (id::hardSync, 468, 300, { 1.0f, 0.0f }, { "ON", "OFF", "HARD SYNC" }, h);

    // noise
    knob (id::noiseLevel, "LEVEL", 585, 196, 20, false, 11, false, h);
    toggle (id::noisePink, 553, 300, { 0.0f, 1.0f }, { "WHITE", "PINK", "COLOR" }, h);
    knob (id::noiseVcf, "VCF MOD", 618, 300, 14, false, 11, false, h);

    // filter
    knob (id::cutoff, "CUTOFF", 706, 196, 24, false, 11, false, h);
    knob (id::resonance, "RESONANCE", 800, 196, 20, false, 11, false, h);
    knob (id::vcfEg, "EG AMOUNT", 706, 300, 18, true, 11, false, h);
    toggle (id::highPass, 800, 300, { 0.0f, 1.0f }, { "LOW", "HIGH", "MODE" }, h);

    // decays
    knob (id::pitchDecay, "PITCH", 896, 196, 18, false, 11, false, h);
    knob (id::vcfDecay, "FILTER", 964, 196, 18, false, 11, false, h);
    knob (id::vcaDecay, "VCA", 1032, 196, 18, false, 11, false, h);
    toggle (id::vcaSlow, 912, 300, { 0.0f, 1.0f }, { "FAST", "SLOW", "VCA ATTACK" }, h);
    knob (id::drive, "DRIVE", 1016, 300, 18, false, 11, false, h);

    // output
    knob (id::volume, "VOLUME", 1130, 200, 24, false, 11, false, h);
    clipLamp = &makeIn<ui::Lamp> (*hood, juce::Colour (0xffff4a35), juce::Colour (0xff4a1410));
    clipLamp->setBounds (1116 - hatchArea.getX(), 282 - hatchArea.getY(), 28, 28);

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

    pageTabs = &make<ui::PageTabs> (processor.getState(), id::viewPage);
    pageTabs->setBounds (1060, (int) seqTop + 8, 112, 22);

    // cables hang over everything else
    patchbay = &make<ui::Patchbay> (processor);
    patchbay->setBounds (getLocalBounds());

    showPage (0);

    // an open hatch (saved with the project) shows up open, without the animation
    serviceOpen = processor.getState().getRawParameterValue (id::serviceOpen)->load() > 0.5f;
    screws = lid = serviceOpen ? 1.0f : 0.0f;
    animator.tick = [this] { animateHatch(); };
    animateHatch();

    startTimerHz (30);
}

ui::Knob& FrontPanel::knob (const juce::String& paramId, const juce::String& label, float x, float y, float radius,
                            bool bipolar, int ticks, bool showValue, juce::Component* parent)
{
    auto& target = parent != nullptr ? *parent : static_cast<juce::Component&> (*this);
    auto& k = makeIn<ui::Knob> (target, processor.getState(), paramId, label, radius, ticks, bipolar, showValue);
    k.place (juce::Point<float> (x, y) - target.getPosition().toFloat() * (parent != nullptr ? 1.0f : 0.0f));
    return k;
}

ui::Toggle& FrontPanel::toggle (const juce::String& paramId, float x, float y, std::vector<float> positions,
                                Labels labels, juce::Component* parent)
{
    auto& target = parent != nullptr ? *parent : static_cast<juce::Component&> (*this);
    auto& t = makeIn<ui::Toggle> (target, processor.getState(), paramId, std::move (positions), std::move (labels));
    t.place (juce::Point<float> (x, y) - target.getPosition().toFloat() * (parent != nullptr ? 1.0f : 0.0f));
    return t;
}

void FrontPanel::setService (bool open)
{
    if (auto* parameter = processor.getState().getParameter (id::serviceOpen))
    {
        parameter->beginChangeGesture();
        parameter->setValueNotifyingHost (open ? 1.0f : 0.0f);
        parameter->endChangeGesture();
    }
}

void FrontPanel::animateHatch()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto dt = lastFrame > 0.0 ? juce::jlimit (0.0, 0.1, (now - lastFrame) / 1000.0) : 0.0;
    lastFrame = now;

    // opening: screws first, then the lid; closing the other way round
    if (serviceOpen)
    {
        if (screws < 1.0f)
            screws = juce::jmin (1.0f, screws + (float) (dt / screwTime));
        else
            lid = juce::jmin (1.0f, lid + (float) (dt / lidTime));
    }
    else
    {
        if (lid > 0.0f)
            lid = juce::jmax (0.0f, lid - (float) (dt / lidTime));
        else
            screws = juce::jmax (0.0f, screws - (float) (dt / screwTime));
    }

    // the lid swings up on its top hinge (accelerating like a real flap)
    const auto swing = lid * lid * (3.0f - 2.0f * lid);
    const auto height = juce::jmax (0.01f, 1.0f - swing);
    hood->setTransform (juce::AffineTransform::scale (1.0f, height, 0.0f, (float) hatchArea.getY()));
    hood->setVisible (lid < 1.0f);
    hood->setScrews (screws);
    hood->setShade (swing);
    board->setLidShadow (lid > 0.0f && lid < 1.0f ? 1.0f - swing : 0.0f);

    const auto done = serviceOpen ? lid >= 1.0f : screws <= 0.0f;

    if (done)
    {
        animator.stopTimer();
        lastFrame = 0.0;
    }
    else if (! animator.isTimerRunning())
    {
        animator.startTimerHz (60);
    }
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

void FrontPanel::paintModules (juce::Graphics& g) const
{
    ui::drawPlate (g, { 20.0f, 132.0f, 168.0f, 240.0f }, "OSCILLATOR 1");
    ui::drawPlate (g, { 196.0f, 132.0f, 168.0f, 240.0f }, "OSCILLATOR 2");
    ui::drawPlate (g, { 372.0f, 132.0f, 140.0f, 240.0f }, "MODULATION");
    ui::drawPlate (g, { 520.0f, 132.0f, 130.0f, 240.0f }, "NOISE");
    ui::drawPlate (g, { 658.0f, 132.0f, 190.0f, 240.0f }, "FILTER");
    ui::drawPlate (g, { 856.0f, 132.0f, 216.0f, 240.0f }, "DECAY");
    ui::drawPlate (g, { 1080.0f, 132.0f, 100.0f, 240.0f }, "OUTPUT");
    ui::drawLabel (g, "CLIP", { 1130.0f, 326.0f }, 12.0f);
}

void FrontPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::frame);
    g.setColour (colours::panel);
    g.fillRoundedRectangle (6.0f, 6.0f, (float) designWidth - 12.0f, (float) designHeight - 12.0f, 7.0f);
    ui::drawHazard (g, { 6.0f, (float) designHeight - 36.0f, (float) designWidth - 12.0f, 30.0f });

    // name plate with the program display on its right
    g.setColour (colours::ink);
    g.fillRoundedRectangle (20.0f, 18.0f, 372.0f, 100.0f, 4.0f);
    g.setColour (colours::plateText);
    g.drawRect (26.0f, 24.0f, 360.0f, 88.0f, 1.0f);
    g.setColour (colours::plateText.withAlpha (0.35f));
    g.drawLine (182.0f, 32.0f, 182.0f, 104.0f, 1.0f);
    g.setColour (colours::hazard);
    g.setFont (ui::stencilFont (46.0f));
    g.drawText ("PS-8", juce::Rectangle<float> (38.0f, 28.0f, 140.0f, 50.0f), juce::Justification::centredLeft, false);
    ui::drawLabel (g, "PERCUSSION SYNTHESIZER", { 39.0f, 86.0f }, 9.5f, colours::plateText, juce::Justification::centredLeft);
    ui::drawLabel (g, "UNIT 08 - CONTROL ROOM B", { 39.0f, 101.0f }, 8.0f, colours::dimText, juce::Justification::centredLeft);

    for (auto c : { juce::Point<float> (30, 28), { 382, 28 }, { 30, 108 }, { 382, 108 } })
        ui::drawScrew (g, c);

    // tempo / step display
    g.setColour (colours::ink);
    g.fillRoundedRectangle (680.0f, 22.0f, 290.0f, 92.0f, 4.0f);
    ui::drawLabel (g, "TEMPO BPM", { 744.0f, 38.0f }, 10.0f, colours::dimText);
    ui::drawLabel (g, "RUN", { 834.0f, 100.0f }, 10.0f, colours::dimText);
    ui::drawLabel (g, "STEP", { 912.0f, 38.0f }, 10.0f, colours::dimText);

    // the module row is painted by the hood (see paintModules)
    ui::drawPlate (g, { 20.0f, seqTop, 1160.0f, 268.0f }, "SEQUENCER");

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

    ui::Patchbay::paintPanel (g, (float) designWidth);
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
    patchbay->update();
    programDisplay->update();
    board->update();

    if (const auto open = state.getRawParameterValue (id::serviceOpen)->load() > 0.5f; open != serviceOpen)
    {
        serviceOpen = open;
        animateHatch();
    }

    const auto now = juce::Time::getMillisecondCounterHiRes();

    if (const auto page = state.getRawParameterValue (id::viewPage)->load() > 0.5f ? 1 : 0; page != shownPage)
        showPage (page);

    const auto step = display.step.load();
    const auto length = juce::roundToInt (state.getRawParameterValue (id::length)->load());
    const auto alarm = now < alarmUntil;

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
        stepColumns[(size_t) i].lamp->setState (i == step, alarm, i < length);

    stepNixie->setText (step >= 0 ? juce::String (step + 1) : juce::String());
    bpmNixie->setText (juce::String (juce::roundToInt (display.bpm.load())));
    runLamp->setOn (display.running.load());

    pageTabs->setPlayingPage (step < 0 ? -1 : step / stepsPerPage);

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
    setResizeLimits (minWidth, minWidth * FrontPanel::designHeight / FrontPanel::designWidth,
                     maxWidth, maxWidth * FrontPanel::designHeight / FrontPanel::designWidth);

    // the last window size, shared by all instances
    const auto width = juce::jlimit (minWidth, maxWidth, settings->getEditorWidth (1020));
    setSize (width, juce::roundToInt ((double) width * FrontPanel::designHeight / FrontPanel::designWidth));
    sizeRestored = true;
}

void PS8AudioProcessorEditor::resized()
{
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) FrontPanel::designWidth));

    if (sizeRestored)
        settings->setEditorWidth (getWidth());
}
