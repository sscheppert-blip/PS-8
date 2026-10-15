#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Pattern memory and step tools. A pattern is the step data (pitch, velocity, on, ratchet, chance
// for all 16 steps) plus the sequence length. The pattern being played always lives in the
// parameters (so it can be automated and edited with the knobs); the other seven wait in slots.
// The tools (shift, copy/paste a page, randomise, undo) act on the current pattern.
// All calls on the message thread, except toTree (any thread).
namespace ps8
{
    class Patterns
    {
    public:
        static constexpr int numPatterns = 8;
        static constexpr int stepsPerPage = 8;

        using Values = std::vector<float>;   // plain values, in the order of paramIds()

        enum class Randomise { pitch, velocity, ratchet, chance, steps, everything };

        explicit Patterns (juce::AudioProcessorValueTreeState&);

        static const juce::StringArray& paramIds();
        static bool isPatternParameter (const juce::String& paramId);

        // patterns
        int current() const;
        void select (int index);
        void copy (int from, int to);
        void clear (int index);
        bool hasContent (int index) const;   // differs from an empty (Init) pattern

        // step tools on the current pattern
        void shift (int direction);           // rotates the steps within LENGTH: -1 left, +1 right
        void copyPage (int page);             // eight steps to the clipboard (shared by all instances)
        bool canPaste() const;
        void pastePage (int page);
        void randomise (Randomise, juce::Random&);
        void clearCurrent();
        bool canUndo() const;
        void undo();                          // undoes the last tool; pressed again, redoes it

        // saved state: all eight patterns and which one plays
        juce::ValueTree toTree() const;
        // takes the slots; the current pattern's values must already be in the parameters
        void fromTree (const juce::ValueTree&);
        void reset();                         // empty slots, pattern 1 plays what the parameters hold

        // everything that matters for "modified" (current index and the other slots)
        Values fingerprint() const;

        Values read() const;
        void write (const Values&);
        Values defaults() const;

    private:
        juce::RangedAudioParameter* parameter (int index) const;
        int length (const Values&) const;
        void remember();                      // snapshot for undo

        juce::AudioProcessorValueTreeState& state;
        std::vector<juce::RangedAudioParameter*> parameters;

        mutable juce::CriticalSection lock;
        std::array<Values, numPatterns> slots;
        int currentIndex = 0;

        Values undoValues;
        bool undoValid = false;
    };
}
