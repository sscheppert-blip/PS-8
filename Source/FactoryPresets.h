#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <utility>
#include <vector>

// Built-in, read-only presets (mostly the sound recipes from the manual). Each one lists only what
// differs from Init, in plain parameter values.
namespace ps8
{
    struct FactoryPreset
    {
        juce::String name;
        std::vector<std::pair<juce::String, float>> values;
        juce::String patch;   // cables, as in the saved state ("vcaEg>tempo …")
        juce::String category;
    };

    const std::vector<FactoryPreset>& factoryPresets();
    juce::ValueTree factoryTree (const FactoryPreset&);
}
