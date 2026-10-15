#include "Presets.h"
#include "FactoryPresets.h"
#include "Parameters.h"
#include "PluginProcessor.h"

#include <optional>

namespace ps8
{
    namespace
    {
        constexpr const char* presetTag = "PS8Preset";
        constexpr const char* paramTag = "PARAM";
        constexpr int presetVersion = 1;

        std::optional<juce::File>& folderOverride()
        {
            static std::optional<juce::File> folder;
            return folder;
        }

        juce::ValueTree readPresetFile (const juce::File& file)
        {
            if (const auto xml = juce::XmlDocument::parse (file); xml != nullptr && xml->hasTagName (presetTag))
                return juce::ValueTree::fromXml (*xml);

            return {};
        }

        juce::String nameOf (const juce::File& file, const juce::ValueTree& tree)
        {
            const auto name = tree.getProperty ("name").toString().trim();
            return name.isNotEmpty() ? name : file.getFileNameWithoutExtension();
        }
    }

    juce::File appDataFolder()
    {
        if (folderOverride().has_value())
            return *folderOverride();

        const auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory);

       #if JUCE_MAC
        return base.getChildFile ("Application Support").getChildFile ("PS-8");
       #else
        return base.getChildFile ("PS-8");   // %APPDATA% on Windows, ~/.config on Linux
       #endif
    }

    void setAppDataFolderOverride (const juce::File& folder)
    {
        folderOverride() = folder;
    }

    //==============================================================================
    Presets::Presets (PS8AudioProcessor& p) : processor (p)
    {
    }

    juce::File Presets::folder()
    {
        return appDataFolder().getChildFile ("Presets");
    }

    juce::File Presets::fileFor (const juce::String& name)
    {
        return folder().getChildFile (juce::File::createLegalFileName (name.trim()) + fileExtension);
    }

    bool Presets::userPresetExists (const juce::String& name)
    {
        return fileFor (name).existsAsFile();
    }

    std::vector<Presets::Entry> Presets::userPresets() const
    {
        std::vector<Entry> entries;

        for (const auto& file : folder().findChildFiles (juce::File::findFiles, false, juce::String ("*") + fileExtension))
            entries.push_back ({ nameOf (file, readPresetFile (file)), false, file });

        std::sort (entries.begin(), entries.end(),
                   [] (const Entry& a, const Entry& b) { return a.name.compareNatural (b.name) < 0; });
        return entries;
    }

    std::vector<Presets::Entry> Presets::list() const
    {
        std::vector<Entry> entries { { initName, true, {} } };

        for (const auto& preset : factoryPresets())
            entries.push_back ({ preset.name, true, {} });

        for (auto& entry : userPresets())
            entries.push_back (std::move (entry));

        return entries;
    }

    bool Presets::isPresetParameter (const juce::String& paramId)
    {
        namespace id = params::id;
        return paramId != id::run && paramId != id::hostClock && paramId != id::viewPage && paramId != id::serviceOpen;
    }

    juce::ValueTree Presets::capture() const
    {
        juce::ValueTree tree (presetTag);
        tree.setProperty ("version", presetVersion, nullptr);

        for (auto* parameter : processor.getParameters())
        {
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (parameter); p != nullptr && isPresetParameter (p->paramID))
            {
                juce::ValueTree child (paramTag);
                child.setProperty ("id", p->paramID, nullptr);
                child.setProperty ("value", p->convertFrom0to1 (p->getValue()), nullptr);
                tree.appendChild (child, nullptr);
            }
        }

        tree.setProperty ("patch", PS8AudioProcessor::patchToString (processor.getPatch()), nullptr);
        return tree;
    }

    void Presets::apply (const juce::ValueTree& tree)
    {
        // parameters missing in the preset go back to their defaults
        for (auto* parameter : processor.getParameters())
        {
            auto* p = dynamic_cast<juce::RangedAudioParameter*> (parameter);

            if (p == nullptr || ! isPresetParameter (p->paramID))
                continue;

            auto value = p->getDefaultValue();
            const auto child = tree.getChildWithProperty ("id", p->paramID);

            if (child.isValid() && child.hasProperty ("value"))
                value = p->convertTo0to1 ((float) child.getProperty ("value"));

            if (std::abs (p->getValue() - value) > 1.0e-7f)
                p->setValueNotifyingHost (value);
        }

        processor.setPatch (PS8AudioProcessor::patchFromString (tree.getProperty ("patch").toString()));
    }

    bool Presets::load (const Entry& entry)
    {
        juce::ValueTree tree;

        if (entry.factory)
        {
            tree = juce::ValueTree (presetTag);

            if (entry.name != initName)
            {
                const auto& all = factoryPresets();
                const auto found = std::find_if (all.begin(), all.end(), [&] (const auto& p) { return entry.name == p.name; });

                if (found == all.end())
                    return false;

                tree = factoryTree (*found);
            }
        }
        else
        {
            tree = readPresetFile (entry.file);

            if (! tree.isValid())
                return false;
        }

        apply (tree);
        setStateName (entry.factory ? entry.name : nameOf (entry.file, tree), entry.factory);
        rebaseline();
        return true;
    }

    void Presets::loadInit()
    {
        load ({ initName, true, {} });
    }

    void Presets::step (int delta)
    {
        const auto entries = list();
        const auto here = current();
        const auto found = std::find (entries.begin(), entries.end(), here);
        const auto count = (int) entries.size();

        auto index = found != entries.end() ? (int) std::distance (entries.begin(), found) + delta
                                            : (delta > 0 ? 0 : count - 1);
        index = ((index % count) + count) % count;
        load (entries[(size_t) index]);
    }

    bool Presets::save (const juce::String& rawName)
    {
        const auto name = rawName.trim();

        if (name.isEmpty() || ! folder().createDirectory())
            return false;

        auto tree = capture();
        tree.setProperty ("name", name, nullptr);

        const auto xml = tree.createXml();

        if (xml == nullptr || ! xml->writeTo (fileFor (name)))
            return false;

        setStateName (name, false);
        rebaseline();
        return true;
    }

    bool Presets::rename (const Entry& entry, const juce::String& rawName)
    {
        const auto name = rawName.trim();

        if (entry.factory || name.isEmpty())
            return false;

        auto tree = readPresetFile (entry.file);

        if (! tree.isValid())
            return false;

        tree.setProperty ("name", name, nullptr);
        const auto target = fileFor (name);
        const auto xml = tree.createXml();

        if (xml == nullptr || ! xml->writeTo (target))
            return false;

        if (target != entry.file)
            entry.file.deleteFile();

        if (current() == entry)
            setStateName (name, false);

        return true;
    }

    bool Presets::remove (const Entry& entry)
    {
        if (entry.factory || ! entry.file.existsAsFile())
            return false;

        return entry.file.moveToTrash() || entry.file.deleteFile();
    }

    Presets::Entry Presets::current() const
    {
        const juce::SpinLock::ScopedLockType lock (nameLock);
        auto entry = currentEntry;

        if (! entry.factory)
            entry.file = fileFor (entry.name);

        return entry;
    }

    juce::String Presets::getStateName() const
    {
        const juce::SpinLock::ScopedLockType lock (nameLock);
        return currentEntry.factory ? currentEntry.name : "user:" + currentEntry.name;
    }

    void Presets::setStateName (const juce::String& name, bool factory)
    {
        const juce::SpinLock::ScopedLockType lock (nameLock);
        currentEntry = { name.isNotEmpty() ? name : juce::String (initName), factory || name.isEmpty(), {} };
    }

    std::vector<float> Presets::currentValues() const
    {
        std::vector<float> values;

        for (auto* parameter : processor.getParameters())
            if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (parameter); p != nullptr && isPresetParameter (p->paramID))
                values.push_back (p->getValue());

        for (const auto mask : processor.getPatch())
            values.push_back ((float) mask);

        return values;
    }

    void Presets::rebaseline()
    {
        baseline = currentValues();
        baselineStateVersion = processor.getStateVersion();
        hasBaseline = true;
    }

    bool Presets::isModified()
    {
        // a freshly loaded project counts as unmodified
        if (! hasBaseline || baselineStateVersion != processor.getStateVersion())
        {
            rebaseline();
            return false;
        }

        const auto values = currentValues();

        if (values.size() != baseline.size())
            return true;

        for (size_t i = 0; i < values.size(); ++i)
            if (std::abs (values[i] - baseline[i]) > 1.0e-5f)
                return true;

        return false;
    }

    //==============================================================================
    UserSettings::UserSettings()
    {
        juce::PropertiesFile::Options options;
        options.storageFormat = juce::PropertiesFile::storeAsXML;
        options.millisecondsBeforeSaving = 500;
        file = std::make_unique<juce::PropertiesFile> (appDataFolder().getChildFile ("settings.xml"), options);
    }

    int UserSettings::getEditorWidth (int fallback) const
    {
        return file->getIntValue ("editorWidth", fallback);
    }

    void UserSettings::setEditorWidth (int width)
    {
        if (file->getIntValue ("editorWidth", -1) == width)
            return;

        appDataFolder().createDirectory();
        file->setValue ("editorWidth", width);
    }
}
