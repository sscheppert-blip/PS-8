#include "Patterns.h"
#include "Parameters.h"

namespace ps8
{
    namespace
    {
        // per step, in this order
        enum Field { pitchField, velocityField, onField, ratchetField, chanceField, numFields };

        constexpr int lengthIndex = seq::maxSteps * numFields;

        int at (int step, Field field) { return step * numFields + field; }

        // copy / paste of one page, shared by all PS-8 instances in the process
        Patterns::Values& clipboard()
        {
            static Patterns::Values page;
            return page;
        }
    }

    const juce::StringArray& Patterns::paramIds()
    {
        static const auto ids = []
        {
            namespace id = params::id;
            juce::StringArray all;

            for (int i = 0; i < seq::maxSteps; ++i)
            {
                all.add (id::stepPitch (i));
                all.add (id::stepVelocity (i));
                all.add (id::stepOn (i));
                all.add (id::stepRatchet (i));
                all.add (id::stepChance (i));
            }

            all.add (id::length);
            return all;
        }();

        return ids;
    }

    bool Patterns::isPatternParameter (const juce::String& paramId)
    {
        return paramIds().contains (paramId);
    }

    Patterns::Patterns (juce::AudioProcessorValueTreeState& s) : state (s)
    {
        for (const auto& paramId : paramIds())
        {
            auto* p = state.getParameter (paramId);
            jassert (p != nullptr);
            parameters.push_back (p);
        }

        reset();
    }

    juce::RangedAudioParameter* Patterns::parameter (int index) const
    {
        return parameters[(size_t) index];
    }

    Patterns::Values Patterns::read() const
    {
        Values values;
        values.reserve (parameters.size());

        for (auto* p : parameters)
            values.push_back (p->convertFrom0to1 (p->getValue()));

        return values;
    }

    void Patterns::write (const Values& values)
    {
        for (size_t i = 0; i < parameters.size() && i < values.size(); ++i)
        {
            auto* p = parameters[i];
            const auto normalised = p->convertTo0to1 (values[i]);

            if (std::abs (p->getValue() - normalised) > 1.0e-7f)
                p->setValueNotifyingHost (normalised);
        }
    }

    Patterns::Values Patterns::defaults() const
    {
        Values values;

        for (auto* p : parameters)
            values.push_back (p->convertFrom0to1 (p->getDefaultValue()));

        return values;
    }

    int Patterns::length (const Values& values) const
    {
        return juce::jlimit (1, seq::maxSteps, juce::roundToInt (values[(size_t) lengthIndex]));
    }

    void Patterns::remember()
    {
        undoValues = read();
        undoValid = true;
    }

    //==============================================================================
    int Patterns::current() const
    {
        const juce::ScopedLock sl (lock);
        return currentIndex;
    }

    void Patterns::select (int index)
    {
        index = juce::jlimit (0, numPatterns - 1, index);
        Values next;

        {
            const juce::ScopedLock sl (lock);

            if (index == currentIndex)
                return;

            slots[(size_t) currentIndex] = read();
            currentIndex = index;
            next = slots[(size_t) index];
        }

        write (next);
        undoValid = false;
    }

    void Patterns::copy (int from, int to)
    {
        from = juce::jlimit (0, numPatterns - 1, from);
        to = juce::jlimit (0, numPatterns - 1, to);

        if (from == to)
            return;

        Values source;
        bool intoCurrent = false;

        {
            const juce::ScopedLock sl (lock);
            source = from == currentIndex ? read() : slots[(size_t) from];
            intoCurrent = to == currentIndex;

            if (! intoCurrent)
                slots[(size_t) to] = source;
        }

        if (intoCurrent)
        {
            remember();
            write (source);
        }
    }

    void Patterns::clear (int index)
    {
        if (index == current())
        {
            clearCurrent();
            return;
        }

        const juce::ScopedLock sl (lock);
        slots[(size_t) juce::jlimit (0, numPatterns - 1, index)] = defaults();
    }

    bool Patterns::hasContent (int index) const
    {
        const auto values = [&]
        {
            const juce::ScopedLock sl (lock);
            return index == currentIndex ? read() : slots[(size_t) juce::jlimit (0, numPatterns - 1, index)];
        }();

        const auto empty = defaults();

        for (size_t i = 0; i < values.size(); ++i)
            if (std::abs (values[i] - empty[i]) > 1.0e-4f)
                return true;

        return false;
    }

    //==============================================================================
    void Patterns::shift (int direction)
    {
        const auto before = read();
        auto after = before;
        const auto len = length (before);

        for (int s = 0; s < len; ++s)
        {
            const auto from = direction < 0 ? (s + 1) % len : (s - 1 + len) % len;

            for (int f = 0; f < numFields; ++f)
                after[(size_t) at (s, (Field) f)] = before[(size_t) at (from, (Field) f)];
        }

        remember();
        write (after);
    }

    void Patterns::copyPage (int page)
    {
        const auto values = read();
        const auto first = juce::jlimit (0, seq::maxSteps / stepsPerPage - 1, page) * stepsPerPage;

        clipboard().assign (values.begin() + at (first, pitchField), values.begin() + at (first + stepsPerPage, pitchField));
    }

    bool Patterns::canPaste() const
    {
        return ! clipboard().empty();
    }

    void Patterns::pastePage (int page)
    {
        if (! canPaste())
            return;

        auto values = read();
        const auto first = juce::jlimit (0, seq::maxSteps / stepsPerPage - 1, page) * stepsPerPage;
        std::copy (clipboard().begin(), clipboard().end(), values.begin() + at (first, pitchField));

        remember();
        write (values);
    }

    void Patterns::randomise (Randomise what, juce::Random& random)
    {
        auto values = read();
        const auto len = length (values);
        const auto all = what == Randomise::everything;

        for (int s = 0; s < len; ++s)
        {
            auto set = [&] (Field f, float v) { values[(size_t) at (s, f)] = v; };

            // whole semitones within an octave either way; SCALE and ROOT still apply on playback
            if (all || what == Randomise::pitch)
                set (pitchField, (float) (random.nextInt (25) - 12) / 24.0f);

            if (all || what == Randomise::velocity)
                set (velocityField, 0.35f + 0.65f * random.nextFloat());

            if (all || what == Randomise::ratchet)
            {
                const auto r = random.nextFloat();
                set (ratchetField, r < 0.6f ? 1.0f : r < 0.85f ? 2.0f : r < 0.95f ? 3.0f : 4.0f);
            }

            if (all || what == Randomise::chance)
                set (chanceField, random.nextBool() ? 1.0f : 0.25f + 0.65f * random.nextFloat());

            // the first step always plays, so the pattern keeps its downbeat
            if (all || what == Randomise::steps)
                set (onField, s == 0 || random.nextFloat() < 0.75f ? 1.0f : 0.0f);
        }

        remember();
        write (values);
    }

    void Patterns::clearCurrent()
    {
        remember();
        write (defaults());
    }

    bool Patterns::canUndo() const
    {
        return undoValid;
    }

    void Patterns::undo()
    {
        if (! undoValid)
            return;

        auto now = read();
        write (undoValues);
        undoValues = std::move (now);
    }

    //==============================================================================
    juce::ValueTree Patterns::toTree() const
    {
        const juce::ScopedLock sl (lock);
        juce::ValueTree tree ("PATTERNS");
        tree.setProperty ("current", currentIndex, nullptr);

        for (int i = 0; i < numPatterns; ++i)
        {
            const auto values = i == currentIndex ? read() : slots[(size_t) i];
            juce::ValueTree pattern ("PATTERN");
            pattern.setProperty ("index", i, nullptr);

            for (int v = 0; v < paramIds().size(); ++v)
                pattern.setProperty (paramIds()[v], values[(size_t) v], nullptr);

            tree.appendChild (pattern, nullptr);
        }

        return tree;
    }

    void Patterns::fromTree (const juce::ValueTree& tree)
    {
        if (! tree.isValid())
        {
            reset();
            return;
        }

        const auto empty = defaults();
        const juce::ScopedLock sl (lock);
        currentIndex = juce::jlimit (0, numPatterns - 1, (int) tree.getProperty ("current", 0));

        for (int i = 0; i < numPatterns; ++i)
        {
            auto values = empty;
            const auto pattern = tree.getChildWithProperty ("index", i);

            for (int v = 0; v < paramIds().size(); ++v)
                if (pattern.hasProperty (paramIds()[v]))
                    values[(size_t) v] = (float) pattern.getProperty (paramIds()[v]);

            slots[(size_t) i] = std::move (values);
        }

        undoValid = false;
    }

    void Patterns::reset()
    {
        const auto empty = defaults();
        const juce::ScopedLock sl (lock);

        for (auto& slot : slots)
            slot = empty;

        currentIndex = 0;
        undoValid = false;
    }

    Patterns::Values Patterns::fingerprint() const
    {
        const juce::ScopedLock sl (lock);
        Values values { (float) currentIndex };

        for (int i = 0; i < numPatterns; ++i)
            if (i != currentIndex)
                values.insert (values.end(), slots[(size_t) i].begin(), slots[(size_t) i].end());

        return values;
    }
}
