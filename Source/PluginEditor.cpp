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

FrontPanel::FrontPanel (PS8AudioProcessor& p, ui::Theme t) : processor (p), theme (t)
{
    ui::setTheme (*this, theme);
    setSize (designWidth, designHeight);
    setOpaque (true);

    // top bar
    programDisplay = &make<ui::ProgramDisplay> (processor);

    if (theme == ui::Theme::matteBlack)
    {
        programDisplay->setTopLeftPosition (214, 27);
        statusDisplay = &make<ui::StatusDisplay> (processor);
        statusDisplay->setBounds (420, 18, 364, 96);
        ledMeter = &make<ui::LedMeter>();
        ledMeter->setBounds (806, 58, 140, 16);
    }
    else
    {
        programDisplay->setTopLeftPosition (194, 27);

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
    }

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
    clipLamp = &makeIn<ui::Lamp> (*hood, theme == ui::Theme::matteBlack ? ui::matte::ledRed : juce::Colour (0xffff4a35),
                                  juce::Colour (0xff4a1410));
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
        column.middle = &knob (id::stepVelocity (i), {}, stepX (i), seqTop + 158.0f, 14);
        column.lamp = &make<ui::LampButton> (processor.getState(), id::stepOn (i));
        column.lamp->place ({ stepX (i), seqTop + 222.0f });
    }

    pageTabs = &make<ui::PageTabs> (processor.getState(), id::viewPage);
    pageTabs->setBounds (1060, (int) seqTop + 8, 112, 22);

    // the clock rate and the sequence length sit in the title bar, mirroring the page tabs
    make<ui::Stepper> (processor.getState(), id::rate, "RATE", 38.0f).setBounds (34, (int) seqTop + 8, 130, 22);
    make<ui::Stepper> (processor.getState(), id::length, "LENGTH", 52.0f).setBounds (180, (int) seqTop + 8, 120, 22);

    // patterns and step tools share the title bar with rate, length and the page tabs
    sequencerTools = &make<ui::SequencerTools> (processor);
    sequencerTools->setTopLeftPosition (312, (int) seqTop + 8);

    // what the middle row of step knobs sets
    stepRowKeys = &make<ui::SelectorKeys> (processor.getState(), id::stepRow,
                                           juce::StringArray { "VELOCITY", "RATCHET", "CHANCE" });
    stepRowKeys->setBounds (190, (int) seqTop + 128, 74, 60);
    showStepRow (stepRowKeys->getSelected());

    // attenuator knobs in the patchbay's utility block
    for (int i = 0; i < 2; ++i)
    {
        const auto c = ui::Patchbay::attenuatorKnob (i);
        knob (i == 0 ? id::att1 : id::att2, {}, c.x, c.y, 11, true);   // labelled by the patchbay
    }

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
        column.middle->setVisible (visible);
        column.lamp->setVisible (visible);
    }

    repaint();
}

void FrontPanel::showStepRow (int row)
{
    shownStepRow = row;

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
    {
        auto& knob = *stepColumns[(size_t) i].middle;

        switch ((ps8::params::StepRow) row)
        {
            case ps8::params::StepRow::ratchet: knob.bind (processor.getState(), id::stepRatchet (i), ps8::params::maxRatchet); break;
            case ps8::params::StepRow::chance:  knob.bind (processor.getState(), id::stepChance (i), 11); break;
            case ps8::params::StepRow::velocity:
            default:                            knob.bind (processor.getState(), id::stepVelocity (i), 11); break;
        }
    }
}

void FrontPanel::paintModules (juce::Graphics& g) const
{
    if (theme == ui::Theme::matteBlack)
    {
        // the cover is part of the chassis: section lines instead of plates
        ui::matte::drawChassis (g, { 6.0f, 6.0f, (float) designWidth - 12.0f, (float) designHeight - 12.0f }, 10.0f);

        struct Section { float x, width; const char* title; };

        for (const auto& section : { Section { 20.0f, 168.0f, "OSCILLATOR 1" }, Section { 196.0f, 168.0f, "OSCILLATOR 2" },
                                     Section { 372.0f, 140.0f, "MODULATION" }, Section { 520.0f, 130.0f, "NOISE" },
                                     Section { 658.0f, 190.0f, "FILTER" }, Section { 856.0f, 216.0f, "DECAY" },
                                     Section { 1080.0f, 100.0f, "OUTPUT" } })
            ui::matte::drawSection (g, { section.x, 138.0f, section.width, 0.0f }, section.title);

        ui::matte::drawLabel (g, "CLIP", { 1130.0f, 322.0f }, 9.0f);
        return;
    }

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
    if (theme == ui::Theme::matteBlack)
        paintMatte (g);
    else
        paintControlRoom (g);
}

void FrontPanel::paintMatte (juce::Graphics& g)
{
    namespace matte = ui::matte;
    const auto w = (float) designWidth, h = (float) designHeight;

    g.fillAll (matte::background);
    matte::drawChassis (g, { 6.0f, 6.0f, w - 12.0f, h - 12.0f }, 10.0f);

    for (auto c : { juce::Point<float> (18.0f, 18.0f), { w - 18.0f, 18.0f }, { 18.0f, h - 18.0f }, { w - 18.0f, h - 18.0f } })
        matte::drawScrew (g, c);

    // name, the dash lit red
    const auto nameFont = matte::font (40.0f, true).withExtraKerningFactor (-0.06f);
    juce::AttributedString name;
    name.append ("PS", nameFont, matte::ink);
    name.append ("-", nameFont, matte::ledRed);
    name.append ("8", nameFont, matte::ink);
    name.draw (g, { 34.0f, 26.0f, 170.0f, 48.0f });
    matte::drawLabel (g, "PERCUSSION", { 36.0f, 84.0f }, 9.0f, matte::inkDim, juce::Justification::centredLeft, false);
    matte::drawLabel (g, "SYNTHESIZER", { 36.0f, 98.0f }, 9.0f, matte::inkDim, juce::Justification::centredLeft, false);

    // level meter
    matte::drawLabel (g, "OUTPUT", { 943.0f, 44.0f }, 9.0f, matte::inkDim, juce::Justification::centredRight);
    matte::drawLabel (g, "-36", { 806.0f, 88.0f }, 8.0f, matte::inkFaint, juce::Justification::centredLeft, false);
    matte::drawLabel (g, "+3 DB", { 943.0f, 88.0f }, 8.0f, matte::inkFaint, juce::Justification::centredRight, false);

    // the module row is painted by the hood (see paintModules); the sequencer's title bar is a dark well
    matte::drawSection (g, { 20.0f, seqTop, 1160.0f, 0.0f }, {});
    const auto bar = juce::Rectangle<float> (28.0f, seqTop + 8.0f, 1144.0f, 22.0f).expanded (2.0f, 1.0f);
    g.setColour (matte::well);
    g.fillRoundedRectangle (bar, 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (bar, 4.0f, 1.0f);

    matte::drawLabel (g, "SEQUENCER", { 312.0f + (ui::SequencerTools::patternsEnd + ui::SequencerTools::toolsStart) * 0.5f, seqTop + 19.0f },
                      10.0f, matte::inkFaint);

    for (int i = 0; i < stepsPerPage; ++i)
        matte::drawLabel (g, juce::String (shownPage * stepsPerPage + i + 1), { stepX (i), seqTop + 46.0f }, 11.0f, matte::inkFaint);

    matte::drawLabel (g, "PITCH", { 230.0f, seqTop + 84.0f }, 9.5f, matte::ink, juce::Justification::centredRight);
    matte::drawLabel (g, "STEP", { 230.0f, seqTop + 222.0f }, 9.5f, matte::ink, juce::Justification::centredRight);

    ui::Patchbay::paintPanel (g, w, theme);

    matte::drawLabel (g, "UNIT 08", { 34.0f, h - 24.0f }, 8.5f, matte::inkFaint, juce::Justification::centredLeft);
    matte::drawLabel (g, "ANALOG PERCUSSION", { w - 34.0f, h - 24.0f }, 8.5f, matte::inkFaint, juce::Justification::centredRight);
}

void FrontPanel::paintControlRoom (juce::Graphics& g)
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
    g.setFont (ui::stencilFont (54.0f));
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
    ui::drawPlate (g, { 20.0f, seqTop, 1160.0f, 268.0f }, {});

    // title between the pattern keys and the step tools
    g.setColour (colours::plateText);
    g.setFont (ui::stencilFont (17.0f));
    g.drawText ("SEQUENCER", juce::Rectangle<float> (312.0f + ui::SequencerTools::patternsEnd, seqTop + 8.0f,
                                                     ui::SequencerTools::toolsStart - ui::SequencerTools::patternsEnd, 22.0f),
                juce::Justification::centred, false);

    for (int i = 0; i < stepsPerPage; ++i)
    {
        g.setColour (colours::ink);
        g.setFont (ui::stencilFont (17.0f));
        g.drawText (juce::String (shownPage * stepsPerPage + i + 1),
                    juce::Rectangle<float> (40.0f, 18.0f).withCentre ({ stepX (i), seqTop + 46.0f }),
                    juce::Justification::centred, false);
    }

    ui::drawLabel (g, "PITCH", { 230.0f, seqTop + 84.0f }, 11.0f, colours::ink, juce::Justification::centredRight);
    ui::drawLabel (g, "STEP ON", { 230.0f, seqTop + 222.0f }, 11.0f, colours::ink, juce::Justification::centredRight);

    ui::Patchbay::paintPanel (g, (float) designWidth, theme);
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

    if (statusDisplay != nullptr)
        statusDisplay->update();

    sequencerTools->update();
    board->update();

    if (const auto open = state.getRawParameterValue (id::serviceOpen)->load() > 0.5f; open != serviceOpen)
    {
        serviceOpen = open;
        animateHatch();
    }

    const auto now = juce::Time::getMillisecondCounterHiRes();

    if (const auto page = state.getRawParameterValue (id::viewPage)->load() > 0.5f ? 1 : 0; page != shownPage)
        showPage (page);

    if (const auto row = stepRowKeys->getSelected(); row != shownStepRow)
        showStepRow (row);

    const auto step = display.step.load();
    const auto length = juce::roundToInt (state.getRawParameterValue (id::length)->load());
    const auto alarm = now < alarmUntil;

    for (int i = 0; i < ps8::seq::maxSteps; ++i)
        stepColumns[(size_t) i].lamp->setState (i == step, alarm, i < length);

    if (stepNixie != nullptr)
    {
        stepNixie->setText (step >= 0 ? juce::String (step + 1) : juce::String());
        bpmNixie->setText (juce::String (juce::roundToInt (display.bpm.load())));
        runLamp->setOn (display.running.load());
    }

    pageTabs->setPlayingPage (step < 0 ? -1 : step / stepsPerPage);

    // output needle: -20 dB at the left stop, 0 dB in the middle, +3 dB at the right stop
    const auto peak = display.peak.exchange (0.0f);
    const auto db = juce::Decibels::gainToDecibels (peak, -60.0f);
    const auto target = db <= 0.0f ? juce::jlimit (0.0f, 0.5f, 0.5f * (db + 20.0f) / 20.0f)
                                   : juce::jmin (1.0f, 0.5f + 0.5f * db / 3.0f);
    outputNeedle += (target - outputNeedle) * (target > outputNeedle ? 0.6f : 0.12f);

    if (peak > 0.99f)
        clipUntil = now + 400.0;

    clipLamp->setOn (now < clipUntil);

    if (outputGauge != nullptr)
    {
        outputGauge->setValue (outputNeedle);
        loadNeedle += ((float) processor.getCpuLoad() - loadNeedle) * 0.2f;
        loadGauge->setValue (loadNeedle);
    }

    // LED meter: -36 dB … 0 dB, the two red segments above
    if (ledMeter != nullptr)
    {
        const auto level = juce::jlimit (0.0f, 1.0f, (db + 36.0f) / 39.0f);
        meterLevel = level > meterLevel ? level : meterLevel + (level - meterLevel) * 0.15f;
        ledMeter->setLevel (meterLevel, now < clipUntil);
    }
}

//==============================================================================
PS8AudioProcessorEditor::PS8AudioProcessorEditor (PS8AudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    buildPanel();
    settings->addChangeListener (this);

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) FrontPanel::designWidth / FrontPanel::designHeight);
    setResizeLimits (minWidth, minWidth * FrontPanel::designHeight / FrontPanel::designWidth,
                     maxWidth, maxWidth * FrontPanel::designHeight / FrontPanel::designWidth);

    // the last window size, shared by all instances
    const auto width = juce::jlimit (minWidth, maxWidth, settings->getEditorWidth (1020));
    setSize (width, juce::roundToInt ((double) width * FrontPanel::designHeight / FrontPanel::designWidth));
    sizeRestored = true;
}

PS8AudioProcessorEditor::~PS8AudioProcessorEditor()
{
    settings->removeChangeListener (this);
}

void PS8AudioProcessorEditor::buildPanel()
{
    theme = ui::themeFromId (settings->getThemeId());
    ui::setTheme (*this, theme);   // for parts outside the panel (value bubbles)

    if (panel != nullptr)
        removeChildComponent (panel.get());

    panel = std::make_unique<FrontPanel> (processor, theme);
    addAndMakeVisible (*panel, 0);   // behind the resize corner
    resized();
}

void PS8AudioProcessorEditor::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (ui::themeFromId (settings->getThemeId()) != theme)
        buildPanel();
}

void PS8AudioProcessorEditor::resized()
{
    if (panel == nullptr)
        return;

    panel->setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) FrontPanel::designWidth));

    if (sizeRestored)
        settings->setEditorWidth (getWidth());
}
