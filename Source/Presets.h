#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PS8AudioProcessor;

// Presets and per-user settings. Everything lives in the user's application data folder:
//   Windows  %APPDATA%\PS-8\            macOS  ~/Library/Application Support/PS-8/
//   Linux    ~/.config/PS-8/
// with the user presets in Presets/*.ps8preset (XML) and settings.xml next to them.
// Factory presets are built in and read-only. All calls on the message thread.
namespace ps8
{
    juce::File appDataFolder();
    // tests only: keep the real folder untouched
    void setAppDataFolderOverride (const juce::File& folder);

    class Presets
    {
    public:
        static constexpr const char* fileExtension = ".ps8preset";
        static constexpr const char* initName = "Init";

        struct Entry
        {
            juce::String name;
            bool factory = false;
            juce::File file;    // user presets only

            bool operator== (const Entry& other) const { return name == other.name && factory == other.factory; }
        };

        explicit Presets (PS8AudioProcessor&);

        static juce::File folder();

        // Init, the factory presets, then the user presets (sorted by name)
        std::vector<Entry> list() const;
        std::vector<Entry> userPresets() const;

        bool load (const Entry&);
        void loadInit();
        // previous/next in list(), wrapping around
        void step (int delta);

        // saves the current sound as a user preset; overwrites a user preset of the same name
        bool save (const juce::String& name);
        bool rename (const Entry&, const juce::String& newName);
        bool remove (const Entry&);
        static bool userPresetExists (const juce::String& name);
        static juce::File fileFor (const juce::String& name);

        Entry current() const;
        // the sound differs from the loaded preset
        bool isModified();

        // The preset name travels with the plugin state (may be called from any thread)
        juce::String getStateName() const;
        void setStateName (const juce::String& name, bool factory);

        // sound + sequencer + cables; not RUN, clock source or the step page
        juce::ValueTree capture() const;
        void apply (const juce::ValueTree&);
        static bool isPresetParameter (const juce::String& paramId);

    private:
        void rebaseline();
        std::vector<float> currentValues() const;

        PS8AudioProcessor& processor;

        mutable juce::SpinLock nameLock;
        Entry currentEntry { initName, true, {} };

        std::vector<float> baseline;
        uint32_t baselineStateVersion = 0;
        bool hasBaseline = false;
    };

    // Per-user settings shared by all instances (window size, panel look). Sends a change message
    // when the look changes, so every open editor follows.
    class UserSettings : public juce::ChangeBroadcaster
    {
    public:
        UserSettings();

        int getEditorWidth (int fallback) const;
        void setEditorWidth (int width);

        juce::String getThemeId() const;
        void setThemeId (const juce::String&);

    private:
        std::unique_ptr<juce::PropertiesFile> file;
    };
}
