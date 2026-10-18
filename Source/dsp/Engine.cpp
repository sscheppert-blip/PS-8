#include "Engine.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace ps8::dsp
{
    namespace
    {
        constexpr float pi = 3.14159265358979f;
        constexpr float outputGain = 2.5f;

        // The VCA shuts off below a small control voltage (knee), like the original's: the level holds
        // longer and then ends cleanly instead of fading out exponentially. Fitted to recordings of the
        // original (knee 5–10 % of full scale). The VCA envelope runs slower by vcaStretch, so a hit
        // still falls 60 dB within the time set on the VCA DECAY knob.
        constexpr float vcaKnee = 0.07f;
        const float vcaStretch = 6.907755f / -std::log (vcaKnee + 0.001f * (1.0f - vcaKnee));

        float vcaGain (float controlVoltage) noexcept
        {
            return std::max (0.0f, controlVoltage - vcaKnee) * (1.0f / (1.0f - vcaKnee));
        }   // init sound peaks around -6 dBFS

        // -60 dB after the given time
        float decayCoef (float seconds, double sampleRate) noexcept
        {
            return (float) std::exp (-6.907755 / (std::max (1.0e-4, (double) seconds) * sampleRate));
        }

        float polyBlep (float t, float dt) noexcept
        {
            if (t < dt)
            {
                t /= dt;
                return t + t - t * t - 1.0f;
            }

            if (t > 1.0f - dt)
            {
                t = (t - 1.0f) / dt;
                return t * t + t + t + 1.0f;
            }

            return 0.0f;
        }

        float shape (float phase, float increment, bool square) noexcept
        {
            if (! square)
                return 1.0f - 4.0f * std::abs (phase - 0.5f);

            const auto dt = std::clamp (std::abs (increment), 1.0e-6f, 0.49f);
            auto half = phase + 0.5f;
            if (half >= 1.0f)
                half -= 1.0f;

            return (phase < 0.5f ? 1.0f : -1.0f) + polyBlep (phase, dt) - polyBlep (half, dt);
        }

        float wrap (float phase) noexcept
        {
            if (phase >= 1.0f || phase < 0.0f)
                phase -= std::floor (phase);

            return phase;
        }
    }

    Tolerances Tolerances::fromSeed (uint32_t seed) noexcept
    {
        // -1 … 1 from a hash of the seed, a different one for each part
        auto spread = [seed] (uint32_t part)
        {
            auto h = seed ^ (part * 0x9e3779b9u);
            h ^= h >> 16; h *= 0x7feb352du;
            h ^= h >> 15; h *= 0x846ca68bu;
            h ^= h >> 16;
            return (float) (h & 0xffffu) / 32767.5f - 1.0f;
        };

        Tolerances t;
        t.vco1Detune = 0.02f * spread (1);    // ±24 cents
        t.vco2Detune = 0.02f * spread (2);
        t.eg1Depth   = 0.06f * spread (3);
        t.eg2Depth   = 0.06f * spread (4);
        t.decay[0]   = 0.10f * spread (5);
        t.decay[1]   = 0.10f * spread (6);
        t.decay[2]   = 0.10f * spread (7);
        t.cutoff     = 0.20f * spread (8);    // ±0.2 octaves
        t.resonance  = 0.08f * spread (9);
        return t;
    }

    float vcoHz (float v) noexcept         { return 8.0f * std::exp2 (10.0f * v); }
    float cutoffHz (float v) noexcept      { return 20.0f * std::exp2 (10.0f * v); }
    float decaySeconds (float v) noexcept  { return 0.003f * std::pow (10.0f / 0.003f, v); }
    float pitchEgOctaves (float e) noexcept { return 5.0f * e * std::abs (e); }
    float vcfEgOctaves (float e) noexcept  { return 8.0f * e * std::abs (e); }
    float stepSemitones (float p) noexcept { return 24.0f * p; }
    float glideSeconds (float g) noexcept  { return g * g; }
    float lfoHz (float v) noexcept         { return 0.05f * std::pow (800.0f, v); }

    void Engine::prepare (double newSampleRate) noexcept
    {
        sampleRate = newSampleRate;
        invSampleRate = (float) (1.0 / sampleRate);
        smoothCoef = (float) (1.0 - std::exp (-1.0 / (0.005 * sampleRate)));

        const auto& p = settings;
        vco1Oct.reset (std::log2 (vcoHz (p.vco1Freq)));
        vco2Oct.reset (std::log2 (vcoHz (p.vco2Freq)));
        cutoffOct.reset (std::log2 (cutoffHz (p.cutoff)));
        level1.reset (p.vco1Level * p.vco1Level);
        level2.reset (p.vco2Level * p.vco2Level);
        levelNoise.reset (p.noiseLevel * p.noiseLevel);
        fm.reset (p.fmOn ? 4.0f * p.fmAmount * p.fmAmount : 0.0f);
        res.reset (4.1f * p.resonance);
        vol.reset (p.volume * p.volume);
        noiseMod.reset (p.noiseVcf * p.noiseVcf);
        driveGain.reset (1.0f + 9.0f * p.drive * p.drive);

        pitchEnv = {};
        vcfEnv = {};
        vca = {};
        osc1 = {};
        osc2 = {};
        lfo = {};
        glidedOctaves = stepOctaves;
        std::fill (std::begin (ladder), std::end (ladder), 0.0f);
        std::fill (std::begin (inputs), std::end (inputs), 0.0f);
        std::fill (std::begin (outputs), std::end (outputs), 0.0f);
        trigEdge = vcoResetEdge = {};
        trigPulseRemaining = 0;
        decayScale = 1.0f;
        driftCoef = (float) (1.0 - std::exp (-1.0 / (1.5 * sampleRate)));
        std::fill (std::begin (drift), std::end (drift), 0.0f);
        std::fill (std::begin (driftTarget), std::end (driftTarget), 0.0f);
        std::fill (std::begin (driftCountdown), std::end (driftCountdown), 0);
        hitPitch = hitDecay = 0.0f;
        muting = false;
        mute = 1.0f;
        extValue = extTarget = 0.0f;
        std::fill (std::begin (pink), std::end (pink), 0.0f);
    }

    void Engine::setPatch (const patch::Masks& newMasks) noexcept
    {
        masks = newMasks;
        numPatched = 0;

        for (int in = 0; in < patch::numInputs; ++in)
        {
            if (masks[(std::size_t) in] != 0)
                patchedInputs[numPatched++] = in;
            else
                inputs[in] = 0.0f;
        }

        if (! isPatched (patch::trigIn))
            trigEdge = {};

        if (! isPatched (patch::vcoResetIn))
            vcoResetEdge = {};
    }

    void Engine::trigger (float newVelocity, float semitones) noexcept
    {
        stepVelocity = std::clamp (newVelocity, 0.0f, 1.0f);
        stepOctaves = semitones / 12.0f;

        if (glideSeconds (settings.glide) < 0.001f)
            glidedOctaves = stepOctaves;

        trigPulseRemaining = std::max (1, (int) (0.005 * sampleRate));
        fireEnvelopes (stepVelocity);
    }

    void Engine::fireEnvelopes (float newVelocity) noexcept
    {
        velocity = std::clamp (newVelocity + inputs[patch::velocityIn], 0.0f, 1.0f);
        muting = false;

        // ANALOG: no two hits are quite the same (pitch, decay, level)
        if (const auto a = settings.analog; a > 0.0f)
        {
            hitPitch = a * 0.004f * analogRandom();
            hitDecay = a * 0.03f * analogRandom();
            velocity = std::clamp (velocity * (1.0f + a * 0.03f * analogRandom()), 0.0f, 1.0f);
        }
        else
        {
            hitPitch = hitDecay = 0.0f;
        }

        // like the original, softer hits also decay a little faster (full velocity: the knob's time)
        decayScale = 0.7f + 0.3f * velocity;
        updateDecays();
        mute = 1.0f;

        pitchEnv.fire();
        vcfEnv.fire();

        // the attack starts from the current level, so retriggers do not click
        // FAST is almost a step, like the original's VCA: that hard onset is the click of its kicks
        const auto attackSeconds = settings.vcaSlowAttack ? 0.012f : 0.00003f;
        vca.attackRemaining = std::max (1, (int) (attackSeconds * (float) sampleRate));
        vca.peak = velocity;
        vca.attackStep = (vca.peak - vca.value) / (float) vca.attackRemaining;
        vca.stage = Vca::Stage::attack;
    }

    void Engine::panic() noexcept
    {
        if (vca.stage != Vca::Stage::idle)
            vca.stage = Vca::Stage::kill;

        muting = true;
    }

    float Engine::renderVca() noexcept
    {
        switch (vca.stage)
        {
            case Vca::Stage::attack:
                vca.value += vca.attackStep;

                if (--vca.attackRemaining <= 0)
                {
                    vca.value = vca.peak;
                    vca.stage = Vca::Stage::decay;
                }
                break;

            case Vca::Stage::decay:
                vca.value *= vca.coef;
                break;

            case Vca::Stage::kill:
                vca.value *= vca.killCoef;
                break;

            case Vca::Stage::idle:
            default:
                return 0.0f;
        }

        if (vca.stage != Vca::Stage::attack && vca.value < 1.0e-5f)
        {
            vca.value = 0.0f;
            vca.stage = Vca::Stage::idle;
        }

        return vca.value;
    }

    float Engine::renderLfo (float increment) noexcept
    {
        auto& l = lfo;
        l.phase += increment;

        if (l.phase >= 1.0f)
        {
            l.phase -= std::floor (l.phase);
            rng ^= rng << 13;
            rng ^= rng >> 17;
            rng ^= rng << 5;
            l.held = (float) (int32_t) rng * (1.0f / 2147483648.0f);
        }

        switch (settings.lfoShape)
        {
            case LfoShape::square:     return l.phase < 0.5f ? 1.0f : -1.0f;
            case LfoShape::saw:        return 1.0f - 2.0f * l.phase;
            case LfoShape::ramp:       return 2.0f * l.phase - 1.0f;
            case LfoShape::sampleHold: return l.held;
            case LfoShape::triangle:
            default:                   return 1.0f - 4.0f * std::abs (l.phase - 0.5f);
        }
    }

    float Engine::noise() noexcept
    {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        const auto white = (float) (int32_t) rng * (1.0f / 2147483648.0f);

        if (! settings.noisePink)
            return white;

        // Paul Kellet's pink filter
        pink[0] = 0.99886f * pink[0] + white * 0.0555179f;
        pink[1] = 0.99332f * pink[1] + white * 0.0750759f;
        pink[2] = 0.96900f * pink[2] + white * 0.1538520f;
        pink[3] = 0.86650f * pink[3] + white * 0.3104856f;
        pink[4] = 0.55000f * pink[4] + white * 0.5329522f;
        pink[5] = -0.7616f * pink[5] - white * 0.0168980f;
        const auto out = pink[0] + pink[1] + pink[2] + pink[3] + pink[4] + pink[5] + pink[6] + white * 0.5362f;
        pink[6] = white * 0.115926f;
        return out * 0.2f;
    }

    float Engine::envelopeCoef (float decayKnob, int envelope) const noexcept
    {
        const auto tolerance = 1.0f + settings.analog * tolerances.decay[envelope] + hitDecay;
        const auto stretch = envelope == 2 ? vcaStretch : 1.0f;
        return decayCoef (decaySeconds (decayKnob) * decayScale * tolerance * stretch, sampleRate);
    }

    void Engine::updateDecays() noexcept
    {
        pitchEnv.coef = envelopeCoef (settings.pitchDecay, 0);
        vcfEnv.coef = envelopeCoef (settings.vcfDecay, 1);
        vca.coef = envelopeCoef (settings.vcaDecay, 2);
    }

    float Engine::analogRandom() noexcept
    {
        analogRng ^= analogRng << 13;
        analogRng ^= analogRng >> 17;
        analogRng ^= analogRng << 5;
        return (float) (int32_t) analogRng * (1.0f / 2147483648.0f);
    }

    // slow random walk, like the temperature drift of the expo converters and the filter
    void Engine::updateDrift() noexcept
    {
        for (int i = 0; i < 3; ++i)
        {
            if (--driftCountdown[i] <= 0)
            {
                driftTarget[i] = analogRandom();
                driftCountdown[i] = (int) (sampleRate * (0.5 + 0.5 * (analogRandom() + 1.0f)));
            }

            drift[i] += (driftTarget[i] - drift[i]) * driftCoef;
        }
    }

    void Engine::render (float* out, int numSamples) noexcept
    {
        beginBlock();
        renderSamples (out, numSamples);
    }

    void Engine::beginBlock() noexcept
    {
        const auto& p = settings;
        const auto fs = (float) sampleRate;
        auto& b = block;

        updateDecays();
        vca.killCoef = decayCoef (0.004f, sampleRate);

        b.target1 = std::log2 (vcoHz (p.vco1Freq));
        b.target2 = std::log2 (vcoHz (p.vco2Freq));
        b.targetCutoff = std::log2 (cutoffHz (p.cutoff));
        const auto a = p.analog;
        b.eg1 = pitchEgOctaves (p.vco1Eg) * (1.0f + a * tolerances.eg1Depth);
        b.eg2 = pitchEgOctaves (p.vco2Eg) * (1.0f + a * tolerances.eg2Depth);
        b.detune1 = a * tolerances.vco1Detune;
        b.detune2 = a * tolerances.vco2Detune;
        b.cutoffOffset = a * tolerances.cutoff;
        b.resonanceScale = 1.0f + a * tolerances.resonance;
        b.pitchDrift = a * 0.01f;     // ±12 cents
        b.cutoffDrift = a * 0.05f;
        b.egVcf = vcfEgOctaves (p.vcfEg);
        const auto glideTime = glideSeconds (p.glide);
        b.glideCoef = glideTime < 0.001f ? 1.0f : (float) (1.0 - std::exp (-1.0 / (glideTime * sampleRate)));
        b.stepOn1 = p.pitchTarget == PitchTarget::both ? 1.0f : 0.0f;
        b.stepOn2 = p.pitchTarget != PitchTarget::off ? 1.0f : 0.0f;
        b.lfoIncrement = p.lfoHz * invSampleRate;
        const auto depth = p.lfoDepth * p.lfoDepth;
        b.lfoPitch1 = p.lfoTarget == LfoTarget::pitch ? 2.0f * depth : 0.0f;
        b.lfoPitch2 = p.lfoTarget == LfoTarget::pitch || p.lfoTarget == LfoTarget::vco2 ? 2.0f * depth : 0.0f;
        b.lfoCutoff = p.lfoTarget == LfoTarget::cutoff ? 4.0f * depth : 0.0f;
        b.lfoFm = p.lfoTarget == LfoTarget::fm ? 2.0f * depth : 0.0f;
        b.lfoVolume = p.lfoTarget == LfoTarget::volume ? p.lfoDepth : 0.0f;
        b.targetFm = p.fmOn ? 4.0f * p.fmAmount * p.fmAmount : 0.0f;
        b.targetNoise = p.noiseLevel * p.noiseLevel;
        b.maxOct = std::log2 (0.45f * fs);
        b.maxCutoffOct = std::log2 (std::min (0.4f * fs, 40000.0f));
    }

    void Engine::renderSamples (float* out, int numSamples) noexcept
    {
        using namespace patch;
        const auto& p = settings;
        const auto& b = block;
        const auto c = smoothCoef;
        const auto extStep = (extTarget - extValue) / (float) std::max (1, numSamples);

        auto cv = [this] (Input in) { return inputs[in]; };
        auto unit = [] (float v) { return std::clamp (v, 0.0f, 1.0f); };
        auto squared = [] (float v) { return v * v; };

        for (int i = 0; i < numSamples; ++i)
        {
            // cables carry the previous sample's outputs, so feedback patches work
            for (int k = 0; k < numPatched; ++k)
            {
                const auto in = patchedInputs[k];
                float sum = 0.0f;

                for (int o = 0; o < numOutputs; ++o)
                    if ((masks[(std::size_t) in] >> o) & 1u)
                        sum += outputs[o];

                inputs[in] = sum;
            }

            if (numPatched > 0)
            {
                if (isPatched (trigIn) && trigEdge.process (cv (trigIn)))
                    fireEnvelopes (stepVelocity);

                if (isPatched (vcoDecayIn))
                    pitchEnv.coef = envelopeCoef (unit (p.pitchDecay + cv (vcoDecayIn)), 0);

                if (isPatched (vcfDecayIn))
                    vcfEnv.coef = envelopeCoef (unit (p.vcfDecay + cv (vcfDecayIn)), 1);

                if (isPatched (vcaDecayIn))
                    vca.coef = envelopeCoef (unit (p.vcaDecay + cv (vcaDecayIn)), 2);

                // VCO RST: both oscillators restart at the triangle's rising zero crossing
                if (isPatched (vcoResetIn) && vcoResetEdge.process (cv (vcoResetIn)))
                    osc1.phase = osc2.phase = 0.25f;
            }

            if (p.analog > 0.0f)
                updateDrift();

            const auto pe = pitchEnv.next();
            const auto fe = vcfEnv.next();
            const auto envelope = renderVca();
            const auto mod = renderLfo (b.lfoIncrement);
            const auto step = glidedOctaves += (stepOctaves - glidedOctaves) * b.glideCoef;

            // oscillators (VCO CV inputs: 1 V/oct)
            const auto oct1 = std::min (vco1Oct.next (b.target1, c) + b.eg1 * pe + b.stepOn1 * step + b.lfoPitch1 * mod
                                            + 5.0f * cv (vco1CvIn) + b.detune1 + b.pitchDrift * drift[0] + hitPitch, b.maxOct);
            const auto oct2 = std::min (vco2Oct.next (b.target2, c) + b.eg2 * pe + b.stepOn2 * step + b.lfoPitch2 * mod
                                            + 5.0f * cv (vco2CvIn) + b.detune2 + b.pitchDrift * drift[1] + hitPitch, b.maxOct);
            const auto targetFm = ! isPatched (fmAmountIn) ? b.targetFm
                                                           : (p.fmOn ? 4.0f * squared (unit (p.fmAmount + cv (fmAmountIn))) : 0.0f);
            const auto inc1 = std::exp2 (oct1) * invSampleRate;
            const auto v1 = shape (osc1.phase, inc1, p.vco1Square);
            const auto inc2 = std::exp2 (oct2) * invSampleRate * (1.0f + std::max (0.0f, fm.next (targetFm, c) + b.lfoFm * mod) * v1);
            const auto v2 = shape (osc2.phase, inc2, p.vco2Square);

            osc1.phase += inc1;
            osc2.phase = wrap (osc2.phase + inc2);

            if (osc1.phase >= 1.0f)
            {
                osc1.phase -= 1.0f;

                if (p.hardSync)
                    osc2.phase = wrap (osc1.phase / inc1 * inc2);
            }

            // EXT IN takes the noise slot: a cable first, then the host's audio, else the noise source
            const auto n = noise();
            extValue += extStep;
            const auto noiseSlot = isPatched (extIn) ? cv (extIn) : (extConnected ? extValue : n);
            const auto targetNoise = ! isPatched (noiseLevelIn) ? b.targetNoise : squared (unit (p.noiseLevel + cv (noiseLevelIn)));
            const auto mix = v1 * level1.next (p.vco1Level * p.vco1Level, c)
                           + v2 * level2.next (p.vco2Level * p.vco2Level, c)
                           + noiseSlot * levelNoise.next (targetNoise, c);

            // VCF MOD is normalled to the noise source; the noise VCF MOD knob sets its depth
            const auto vcfMod = isPatched (vcfModIn) ? cv (vcfModIn) : n;

            // 4-pole ladder (zero-delay feedback, saturating input stage)
            const auto cutoffOctave = std::clamp (cutoffOct.next (b.targetCutoff, c) + b.egVcf * fe
                                                      + 3.0f * noiseMod.next (p.noiseVcf * p.noiseVcf, c) * vcfMod + b.lfoCutoff * mod
                                                      + b.cutoffOffset + b.cutoffDrift * drift[2] + 5.0f * cv (vcfCvIn),
                                                  3.3f, b.maxCutoffOct);
            const auto g = std::tan (pi * std::exp2 (cutoffOctave) * invSampleRate);
            const auto G = g / (1.0f + g);
            const auto k = res.next (4.1f * p.resonance * b.resonanceScale, c);
            const auto oneMinusG = 1.0f - G;
            const auto sigma = G * G * G * oneMinusG * ladder[0] + G * G * oneMinusG * ladder[1] + G * oneMinusG * ladder[2] + oneMinusG * ladder[3];
            // a trace of noise lets the filter start ringing on its own, like the circuit does
            const auto u = std::tanh ((0.8f * mix + 1.0e-4f * n - k * sigma) / (1.0f + k * G * G * G * G));

            float y[4];
            auto x = u;

            for (int stage = 0; stage < 4; ++stage)
            {
                const auto v = (x - ladder[stage]) * G;
                y[stage] = v + ladder[stage];
                ladder[stage] = y[stage] + v;
                x = y[stage];
            }

            const auto filtered = p.highPass ? u - 4.0f * y[0] + 6.0f * y[1] - 4.0f * y[2] + y[3]
                                             : y[3] * (1.0f + 0.3f * k);

            const auto dg = driveGain.next (1.0f + 9.0f * p.drive * p.drive, c);
            const auto driven = std::tanh (filtered * dg) / std::sqrt (dg);

            // VCA CV is normalled to the VCA envelope; SCRAM also fades a cable-driven VCA
            if (muting)
                mute *= vca.killCoef;

            const auto amp = isPatched (vcaCvIn) ? vcaGain (unit (cv (vcaCvIn))) * mute : vcaGain (envelope);
            const auto tremolo = 1.0f - b.lfoVolume * (0.5f - 0.5f * mod);
            const auto vcaSignal = driven * amp * tremolo;
            out[i] = outputGain * vcaSignal * vol.next (p.volume * p.volume, c);

            // patchbay outputs
            outputs[trigOut] = trigPulseRemaining > 0 ? 1.0f : 0.0f;
            trigPulseRemaining = std::max (0, trigPulseRemaining - 1);
            outputs[velocityOut] = stepVelocity;
            outputs[pitchOut] = stepOctaves / 5.0f;
            outputs[vco1Out] = v1;
            outputs[vco2Out] = v2;
            outputs[vcoEgOut] = pe;
            outputs[vcfEgOut] = fe;
            outputs[vcaOut] = vcaSignal;
            outputs[vcaEgOut] = envelope;
            outputs[lfoOut] = mod;

            // attenuators: an empty input is normalled to +5 V, so the knob alone gives a set voltage
            outputs[att1Out] = (isPatched (att1In) ? cv (att1In) : 1.0f) * p.att1;
            outputs[att2Out] = (isPatched (att2In) ? cv (att2In) : 1.0f) * p.att2;
        }
    }
}
