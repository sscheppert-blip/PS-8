#include "ProgramDisplay.h"
#include "FactoryPresets.h"
#include "Widgets.h"

namespace ps8::ui
{
    namespace
    {
        enum MenuId
        {
            initId = 1, factoryBase = 100, userBase = 1000,
            saveAsId = 9001, renameId, deleteId, openFolderId
        };

        // a small modal dialog asking for a preset name
        void askForName (juce::Component* owner, const juce::String& title, const juce::String& initial,
                         const juce::String& okText, std::function<void (juce::String)> onOk)
        {
            auto* window = new juce::AlertWindow (title, "Preset name:", juce::MessageBoxIconType::NoIcon, owner);
            window->addTextEditor ("name", initial);
            window->addButton (okText, 1, juce::KeyPress (juce::KeyPress::returnKey));
            window->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

            window->enterModalState (true, juce::ModalCallbackFunction::create ([window, onOk] (int result)
            {
                const auto name = window->getTextEditorContents ("name").trim();

                if (result == 1 && name.isNotEmpty())
                    onOk (name);
            }), true);
        }

        void confirm (juce::Component* owner, const juce::String& title, const juce::String& message,
                      const juce::String& okText, std::function<void()> onOk)
        {
            juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                              .withIconType (juce::MessageBoxIconType::WarningIcon)
                                              .withTitle (title)
                                              .withMessage (message)
                                              .withButton (okText)
                                              .withButton ("Cancel")
                                              .withAssociatedComponent (owner),
                                          [onOk] (int result)
                                          {
                                              if (result == 1)
                                                  onOk();
                                          });
        }
    }

    ProgramDisplay::ProgramDisplay (PS8AudioProcessor& p) : processor (p), presets (p.getPresets())
    {
        setSize (width, height);
        update();
    }

    void ProgramDisplay::update()
    {
        const auto entry = presets.current();
        const auto modified = presets.isModified();

        if (entry.name != shownName || modified != shownModified || entry.factory != shownFactory)
        {
            shownName = entry.name;
            shownModified = modified;
            shownFactory = entry.factory;
            repaint();
        }
    }

    juce::Rectangle<float> ProgramDisplay::areaOf (Key key) const
    {
        switch (key)
        {
            case Key::name:     return { 0.0f, 15.0f, (float) width, 30.0f };
            case Key::previous: return { 0.0f, 54.0f, 30.0f, 24.0f };
            case Key::next:     return { 35.0f, 54.0f, 30.0f, 24.0f };
            case Key::save:     return { 70.0f, 54.0f, 50.0f, 24.0f };
            case Key::menu:     return { 125.0f, 54.0f, 51.0f, 24.0f };
            case Key::none:     break;
        }

        return {};
    }

    ProgramDisplay::Key ProgramDisplay::keyAt (juce::Point<float> position) const
    {
        for (const auto key : { Key::name, Key::previous, Key::next, Key::save, Key::menu })
            if (areaOf (key).contains (position))
                return key;

        return Key::none;
    }

    void ProgramDisplay::paint (juce::Graphics& g)
    {
        drawLabel (g, "PROGRAM", { 0.0f, 6.0f }, 9.0f, colours::dimText, juce::Justification::centredLeft);
        drawLabel (g, shownFactory ? "FACTORY" : "USER", { (float) width, 6.0f }, 9.0f, colours::dimText,
                   juce::Justification::centredRight);

        // glass window
        const auto window = areaOf (Key::name);
        g.setColour (juce::Colour (0xff0b100d));
        g.fillRoundedRectangle (window, 3.0f);
        g.setColour (colours::plateShade);
        g.drawRoundedRectangle (window.reduced (0.5f), 3.0f, 1.0f);

        const auto text = window.reduced (8.0f, 0.0f).withTrimmedRight (12.0f);
        g.setFont (labelFont (16.0f));
        g.setColour (colours::nixie.withAlpha (0.3f));
        g.drawText (shownName.toUpperCase() + (shownModified ? " *" : ""), text.translated (0.0f, 1.0f),
                    juce::Justification::centredLeft, true);
        g.setColour (colours::nixie);
        g.drawText (shownName.toUpperCase() + (shownModified ? " *" : ""), text, juce::Justification::centredLeft, true);

        juce::Path arrow;
        const auto a = juce::Point<float> (window.getRight() - 12.0f, window.getCentreY());
        arrow.addTriangle (a.x - 4.0f, a.y - 2.5f, a.x + 4.0f, a.y - 2.5f, a.x, a.y + 3.0f);
        g.setColour (colours::nixie.withAlpha (0.7f));
        g.fillPath (arrow);

        // keys
        for (const auto key : { Key::previous, Key::next, Key::save, Key::menu })
        {
            const auto area = areaOf (key);
            const auto down = pressed == key;
            g.setColour (juce::Colours::black.withAlpha (0.4f));
            g.fillRoundedRectangle (area.translated (0.0f, 1.5f), 3.0f);
            g.setColour (down ? juce::Colour (0xff1c221e) : juce::Colour (0xff343c36));
            g.fillRoundedRectangle (area.translated (0.0f, down ? 1.0f : 0.0f), 3.0f);
            g.setColour (juce::Colours::white.withAlpha (down ? 0.04f : 0.12f));
            g.drawLine (area.getX() + 3.0f, area.getY() + 1.0f, area.getRight() - 3.0f, area.getY() + 1.0f, 1.0f);

            const auto c = area.getCentre().translated (0.0f, down ? 1.0f : 0.0f);
            g.setColour (colours::plateText);

            if (key == Key::previous || key == Key::next)
            {
                const auto s = key == Key::next ? 1.0f : -1.0f;
                juce::Path triangle;
                triangle.addTriangle (c.x - 4.0f * s, c.y - 5.0f, c.x - 4.0f * s, c.y + 5.0f, c.x + 5.0f * s, c.y);
                g.fillPath (triangle);
            }
            else
            {
                g.setFont (stencilFont (13.0f));
                g.drawText (key == Key::save ? "SAVE" : "MENU", area.translated (0.0f, down ? 1.0f : 0.0f),
                            juce::Justification::centred, false);
            }
        }
    }

    void ProgramDisplay::mouseDown (const juce::MouseEvent& e)
    {
        pressed = keyAt (e.position);
        repaint();
    }

    void ProgramDisplay::mouseUp (const juce::MouseEvent& e)
    {
        const auto key = pressed;
        pressed = Key::none;
        repaint();

        if (key == Key::none || keyAt (e.position) != key)
            return;

        switch (key)
        {
            case Key::previous: presets.step (-1); break;
            case Key::next:     presets.step (1); break;
            case Key::save:     askToSave(); break;
            case Key::menu:
            case Key::name:     showMenu(); break;
            case Key::none:     break;
        }

        update();
    }

    void ProgramDisplay::showMenu()
    {
        const auto here = presets.current();
        const auto& factory = factoryPresets();
        const auto user = presets.userPresets();
        const auto isUserPreset = ! here.factory && here.file.existsAsFile();

        juce::PopupMenu menu;
        menu.addItem (initId, Presets::initName, true, here.factory && here.name == Presets::initName);
        menu.addSectionHeader ("FACTORY");

        // one submenu per category, in the order the presets are listed
        for (int i = 0; i < (int) factory.size();)
        {
            const auto category = factory[(size_t) i].category;
            juce::PopupMenu sub;
            bool holdsCurrent = false;

            for (; i < (int) factory.size() && factory[(size_t) i].category == category; ++i)
            {
                const auto ticked = here.factory && here.name == factory[(size_t) i].name;
                holdsCurrent = holdsCurrent || ticked;
                sub.addItem (factoryBase + i, factory[(size_t) i].name, true, ticked);
            }

            menu.addSubMenu (category, sub, true, nullptr, holdsCurrent);
        }

        menu.addSectionHeader ("USER");

        if (user.empty())
            menu.addItem (-1, "(none yet - press SAVE)", false);

        for (int i = 0; i < (int) user.size(); ++i)
            menu.addItem (userBase + i, user[(size_t) i].name, true, here == user[(size_t) i]);

        menu.addSeparator();
        menu.addItem (saveAsId, "Save as...");
        menu.addItem (renameId, "Rename...", isUserPreset);
        menu.addItem (deleteId, "Delete", isUserPreset);
        menu.addItem (openFolderId, "Open preset folder");

        menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                            [safe = juce::Component::SafePointer<ProgramDisplay> (this), user] (int result)
        {
            if (safe == nullptr || result == 0)
                return;

            auto& self = *safe;

            if (result == initId)
                self.presets.loadInit();
            else if (result >= factoryBase && result < factoryBase + (int) factoryPresets().size())
                self.presets.load ({ factoryPresets()[(size_t) (result - factoryBase)].name, true, {} });
            else if (result >= userBase && result < userBase + (int) user.size())
                self.presets.load (user[(size_t) (result - userBase)]);
            else if (result == saveAsId)
                self.askToSave();
            else if (result == renameId)
                self.askToRename();
            else if (result == deleteId)
                self.askToDelete();
            else if (result == openFolderId)
            {
                Presets::folder().createDirectory();
                Presets::folder().startAsProcess();
            }

            self.update();
        });
    }

    void ProgramDisplay::askToSave()
    {
        const auto here = presets.current();

        askForName (this, "SAVE PRESET", here.name == Presets::initName ? juce::String() : here.name, "Save",
                    [safe = juce::Component::SafePointer<ProgramDisplay> (this)] (juce::String name)
        {
            if (safe == nullptr)
                return;

            if (Presets::userPresetExists (name))
                confirm (safe, "OVERWRITE PRESET", "A preset called \"" + name + "\" already exists. Overwrite it?", "Overwrite",
                         [safe, name] { if (safe != nullptr) safe->saveAs (name); });
            else
                safe->saveAs (name);
        });
    }

    void ProgramDisplay::saveAs (const juce::String& name)
    {
        if (! presets.save (name))
            juce::AlertWindow::showAsync (juce::MessageBoxOptions()
                                              .withIconType (juce::MessageBoxIconType::WarningIcon)
                                              .withTitle ("SAVE FAILED")
                                              .withMessage ("The preset could not be written to\n" + Presets::folder().getFullPathName())
                                              .withButton ("OK")
                                              .withAssociatedComponent (this),
                                          nullptr);
        update();
    }

    void ProgramDisplay::askToRename()
    {
        const auto here = presets.current();

        askForName (this, "RENAME PRESET", here.name, "Rename",
                    [safe = juce::Component::SafePointer<ProgramDisplay> (this), here] (juce::String name)
        {
            if (safe == nullptr || name == here.name)
                return;

            auto doRename = [safe, here, name]
            {
                if (safe != nullptr)
                {
                    safe->presets.rename (here, name);
                    safe->update();
                }
            };

            if (Presets::userPresetExists (name) && Presets::fileFor (name) != here.file)
                confirm (safe, "OVERWRITE PRESET", "A preset called \"" + name + "\" already exists. Overwrite it?", "Overwrite", doRename);
            else
                doRename();
        });
    }

    void ProgramDisplay::askToDelete()
    {
        const auto here = presets.current();

        confirm (this, "DELETE PRESET", "Delete the preset \"" + here.name + "\"? It goes to the trash.", "Delete",
                 [safe = juce::Component::SafePointer<ProgramDisplay> (this), here]
        {
            if (safe != nullptr)
            {
                safe->presets.remove (here);
                safe->update();
            }
        });
    }
}
