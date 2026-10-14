#include "Engine.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace ps8::dsp
{
    namespace
    {
        constexpr float pi = 3.14159265358979f;
        constexpr float outputGain = 2.5f;   // init sound peaks around -6 dBFS

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
        std::fill (std::begin (pink), std::end (pink), 0.0f);
    }

    void Engine::trigger (float newVelocity, float semitones) noexcept
    {
        velocity = std::clamp (newVelocity, 0.0f, 1.0f);
        stepOctaves = semitones / 12.0f;

        if (glideSeconds (settings.glide) < 0.001f)
            glidedOctaves = stepOctaves;

        pitchEnv.fire();
        vcfEnv.fire();

        // the attack starts from the current level, so retriggers do not click
        const auto attackSeconds = settings.vcaSlowAttack ? 0.012f : 0.0003f;
        vca.attackRemaining = std::max (1, (int) (attackSeconds * (float) sampleRate));
        vca.peak = velocity;
        vca.attackStep = (vca.peak - vca.value) / (float) vca.attackRemaining;
        vca.stage = Vca::Stage::attack;
    }

    void Engine::panic() noexcept
    {
        if (vca.stage != Vca::Stage::idle)
            vca.stage = Vca::Stage::kill;
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

    void Engine::render (float* out, int numSamples) noexcept
    {
        const auto& p = settings;
        const auto fs = (float) sampleRate;

        pitchEnv.coef = decayCoef (decaySeconds (p.pitchDecay), sampleRate);
        vcfEnv.coef = decayCoef (decaySeconds (p.vcfDecay), sampleRate);
        vca.coef = decayCoef (decaySeconds (p.vcaDecay), sampleRate);
        vca.killCoef = decayCoef (0.004f, sampleRate);

        const auto target1 = std::log2 (vcoHz (p.vco1Freq));
        const auto target2 = std::log2 (vcoHz (p.vco2Freq));
        const auto targetCutoff = std::log2 (cutoffHz (p.cutoff));
        const auto eg1 = pitchEgOctaves (p.vco1Eg);
        const auto eg2 = pitchEgOctaves (p.vco2Eg);
        const auto egVcf = vcfEgOctaves (p.vcfEg);
        const auto glideTime = glideSeconds (p.glide);
        const auto glideCoef = glideTime < 0.001f ? 1.0f : (float) (1.0 - std::exp (-1.0 / (glideTime * sampleRate)));
        const auto stepOn1 = p.pitchTarget == PitchTarget::both ? 1.0f : 0.0f;
        const auto stepOn2 = p.pitchTarget != PitchTarget::off ? 1.0f : 0.0f;
        const auto lfoIncrement = p.lfoHz * invSampleRate;
        const auto depth = p.lfoDepth * p.lfoDepth;
        const auto lfoPitch1 = p.lfoTarget == LfoTarget::pitch ? 2.0f * depth : 0.0f;
        const auto lfoPitch2 = p.lfoTarget == LfoTarget::pitch || p.lfoTarget == LfoTarget::vco2 ? 2.0f * depth : 0.0f;
        const auto lfoCutoff = p.lfoTarget == LfoTarget::cutoff ? 4.0f * depth : 0.0f;
        const auto lfoFm = p.lfoTarget == LfoTarget::fm ? 2.0f * depth : 0.0f;
        const auto lfoVolume = p.lfoTarget == LfoTarget::volume ? p.lfoDepth : 0.0f;
        const auto targetFm = p.fmOn ? 4.0f * p.fmAmount * p.fmAmount : 0.0f;
        const auto maxOct = std::log2 (0.45f * fs);
        const auto maxCutoffOct = std::log2 (std::min (0.4f * fs, 40000.0f));
        const auto c = smoothCoef;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto pe = pitchEnv.next();
            const auto fe = vcfEnv.next();
            const auto amp = renderVca();
            const auto mod = renderLfo (lfoIncrement);
            const auto step = glidedOctaves += (stepOctaves - glidedOctaves) * glideCoef;

            // oscillators
            const auto oct1 = std::min (vco1Oct.next (target1, c) + eg1 * pe + stepOn1 * step + lfoPitch1 * mod, maxOct);
            const auto oct2 = std::min (vco2Oct.next (target2, c) + eg2 * pe + stepOn2 * step + lfoPitch2 * mod, maxOct);
            const auto inc1 = std::exp2 (oct1) * invSampleRate;
            const auto v1 = shape (osc1.phase, inc1, p.vco1Square);
            const auto inc2 = std::exp2 (oct2) * invSampleRate * (1.0f + std::max (0.0f, fm.next (targetFm, c) + lfoFm * mod) * v1);
            const auto v2 = shape (osc2.phase, inc2, p.vco2Square);

            osc1.phase += inc1;
            osc2.phase = wrap (osc2.phase + inc2);

            if (osc1.phase >= 1.0f)
            {
                osc1.phase -= 1.0f;

                if (p.hardSync)
                    osc2.phase = wrap (osc1.phase / inc1 * inc2);
            }

            const auto n = noise();
            const auto mix = v1 * level1.next (p.vco1Level * p.vco1Level, c)
                           + v2 * level2.next (p.vco2Level * p.vco2Level, c)
                           + n * levelNoise.next (p.noiseLevel * p.noiseLevel, c);

            // 4-pole ladder (zero-delay feedback, saturating input stage)
            const auto cutoffOctave = std::clamp (cutoffOct.next (targetCutoff, c) + egVcf * fe
                                                      + 3.0f * noiseMod.next (p.noiseVcf * p.noiseVcf, c) * n + lfoCutoff * mod,
                                                  3.3f, maxCutoffOct);
            const auto g = std::tan (pi * std::exp2 (cutoffOctave) * invSampleRate);
            const auto G = g / (1.0f + g);
            const auto k = res.next (4.1f * p.resonance, c);
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

            const auto tremolo = 1.0f - lfoVolume * (0.5f - 0.5f * mod);
            out[i] = outputGain * driven * amp * tremolo * vol.next (p.volume * p.volume, c);
        }
    }
}
