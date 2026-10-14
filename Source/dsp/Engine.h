#pragma once

#include <cstdint>

// Percussion voice: two VCOs (triangle/square, linear FM 1→2, hard sync), white/pink noise,
// mixer, 4-pole ladder filter (low/high pass), three decay envelopes (pitch, filter, VCA),
// glide on the step pitch, an LFO, drive and output level. JUCE-free; runs at whatever (oversampled) rate it is prepared with.
namespace ps8::dsp
{
    enum class PitchTarget { both, off, vco2 };
    enum class LfoShape { triangle, square, saw, ramp, sampleHold };
    enum class LfoTarget { pitch, vco2, cutoff, fm, volume };

    struct Settings
    {
        float vco1Freq = 0.30f, vco1Eg = 0.45f, vco1Level = 0.85f;   // freq/level 0..1, eg -1..1
        bool  vco1Square = false;
        float vco2Freq = 0.45f, vco2Eg = 0.30f, vco2Level = 0.35f;
        bool  vco2Square = true;
        float fmAmount = 0.0f;                                       // 0..1
        bool  fmOn = true, hardSync = false;
        float noiseLevel = 0.1f, noiseVcf = 0.0f;                    // 0..1
        bool  noisePink = false;
        float cutoff = 0.55f, resonance = 0.3f, vcfEg = 0.4f;        // vcfEg -1..1
        bool  highPass = false;
        float pitchDecay = 0.3f, vcfDecay = 0.4f, vcaDecay = 0.5f;   // 0..1
        bool  vcaSlowAttack = false;
        float drive = 0.0f, volume = 0.7f;                           // 0..1
        PitchTarget pitchTarget = PitchTarget::both;
        float glide = 0.0f;                                          // 0..1
        float lfoHz = 2.0f, lfoDepth = 0.0f;                         // depth 0..1
        LfoShape lfoShape = LfoShape::triangle;
        LfoTarget lfoTarget = LfoTarget::cutoff;
    };

    // Control mappings (also used for the parameter displays and the tests).
    float vcoHz (float v) noexcept;                // 8 Hz .. 8.2 kHz
    float cutoffHz (float v) noexcept;             // 20 Hz .. 20.5 kHz
    float decaySeconds (float v) noexcept;         // 3 ms .. 10 s (to -60 dB)
    float pitchEgOctaves (float e) noexcept;       // ±5 octaves, finer around the centre
    float vcfEgOctaves (float e) noexcept;         // ±8 octaves, finer around the centre
    float stepSemitones (float p) noexcept;        // ±24 semitones
    float glideSeconds (float g) noexcept;         // 0 .. 1 s
    float lfoHz (float v) noexcept;                // 0.05 .. 40 Hz

    class Engine
    {
    public:
        void prepare (double sampleRate) noexcept;
        void setSettings (const Settings& s) noexcept { settings = s; }

        // velocity 0..1, step pitch in semitones
        void trigger (float newVelocity, float semitones) noexcept;

        // phase 0..1, for an LFO locked to the host's beat
        void setLfoPhase (float phase) noexcept { lfo.phase = phase; }
        float getLfoPhase() const noexcept { return lfo.phase; }

        // fast fade to silence; the next trigger plays again
        void panic() noexcept;

        void render (float* out, int numSamples) noexcept;

        bool isSilent() const noexcept { return vca.value < 1.0e-5f && vca.stage == Vca::Stage::idle; }

    private:
        struct Smoothed
        {
            float value = 0.0f;
            void reset (float v) noexcept { value = v; }
            float next (float target, float coef) noexcept { return value += (target - value) * coef; }
        };

        struct Decay
        {
            float value = 0.0f, coef = 0.0f;
            void fire() noexcept { value = 1.0f; }
            float next() noexcept { const auto v = value; value *= coef; return v; }
        };

        struct Vca
        {
            enum class Stage { idle, attack, decay, kill };
            Stage stage = Stage::idle;
            float value = 0.0f, peak = 0.0f, attackStep = 0.0f, coef = 0.0f, killCoef = 0.0f;
            int attackRemaining = 0;
        };

        struct Oscillator
        {
            float phase = 0.0f;
        };

        struct Lfo
        {
            float phase = 0.0f, held = 0.0f;
        };

        float renderVca() noexcept;
        float renderLfo (float increment) noexcept;
        float noise() noexcept;

        Settings settings;
        double sampleRate = 192000.0;
        float invSampleRate = 1.0f / 192000.0f;
        float smoothCoef = 0.01f;

        Smoothed vco1Oct, vco2Oct, cutoffOct, level1, level2, levelNoise, fm, res, vol, noiseMod, driveGain;
        Decay pitchEnv, vcfEnv;
        Vca vca;
        Oscillator osc1, osc2;
        Lfo lfo;

        float stepOctaves = 0.0f, glidedOctaves = 0.0f, velocity = 1.0f;

        // ladder filter states
        float ladder[4] {};

        // noise
        uint32_t rng = 0x12345678u;
        float pink[7] {};
    };
}
