#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Panel looks. Every theme shares the same layout (design coordinates, jacks, hatch); only the
// painting differs. The front panel stores its theme as a component property, so every part on it
// finds the theme by walking up its parents (themeOf). A new theme = a new enum value, a name, and
// a branch in the paint code of the parts that look different.
namespace ps8::ui
{
    enum class Theme
    {
        controlRoom,   // the original 1970s control-room panel
        matteBlack,    // matte black, rubber keys, OLED
        numThemes
    };

    inline constexpr Theme defaultTheme = Theme::matteBlack;

    juce::String themeName (Theme);   // shown in the menu
    juce::String themeId (Theme);     // stored in the settings
    Theme themeFromId (const juce::String&, Theme fallback = defaultTheme);

    void setTheme (juce::Component&, Theme);
    Theme themeOf (const juce::Component*);
    inline bool isMatte (const juce::Component* c) { return themeOf (c) == Theme::matteBlack; }

    // Matte black: colours and drawing helpers
    namespace matte
    {
        inline const juce::Colour background    { 0xff0a0a0b };
        inline const juce::Colour chassisTop    { 0xff1d1e20 };
        inline const juce::Colour chassisBottom { 0xff101112 };
        inline const juce::Colour ink           { 0xffe9e7e2 };
        inline const juce::Colour inkDim        { 0xff8d8b87 };
        inline const juce::Colour inkFaint      { 0xff5d5b57 };
        inline const juce::Colour line          { 0xff2a2b2d };
        inline const juce::Colour well          { 0xff0b0b0c };   // recessed areas
        inline const juce::Colour ledRed        { 0xffff3b2f };
        inline const juce::Colour ledAmber      { 0xffffb340 };
        inline const juce::Colour ledWhite      { 0xfff4f1ea };
        inline const juce::Colour ledOff        { 0xff2a1312 };
        inline const juce::Colour oled          { 0xff9fd7ff };
        inline const juce::Colour pcb           { 0xff121314 };
        inline const juce::Colour gold          { 0xffb8933f };
        inline const juce::Colour silkscreen    { 0xffd9d6cf };

        juce::Font font (float height, bool bold = false);

        // small spaced capitals
        void drawLabel (juce::Graphics&, const juce::String& text, juce::Point<float> centre, float height,
                        juce::Colour = inkDim, juce::Justification = juce::Justification::centred, bool bold = true);

        // chassis background with a soft diagonal sheen
        void drawChassis (juce::Graphics&, juce::Rectangle<float> area, float cornerSize);
        // section heading: a hairline with the title under it
        void drawSection (juce::Graphics&, juce::Rectangle<float> area, const juce::String& title);
        void drawScrew (juce::Graphics&, juce::Point<float> centre, float radius = 4.5f);

        // rubber key; returns the face (moved down while pressed)
        juce::Rectangle<float> drawKey (juce::Graphics&, juce::Rectangle<float> area, bool down, float cornerSize = 4.0f);
        // LED bar or dot
        void drawLed (juce::Graphics&, juce::Rectangle<float> area, juce::Colour colour, bool on);

        // knob with a thin value ring; angles in radians, clockwise from 12 o'clock
        void drawKnob (juce::Graphics&, juce::Point<float> centre, float radius, float angle,
                       float startAngle, float endAngle, bool bipolar);
        // silver trimmer on the black service board
        void drawTrimmer (juce::Graphics&, juce::Point<float> centre, float radius, float angle,
                          int numTicks, float startAngle, float endAngle);
        void drawJack (juce::Graphics&, juce::Point<float> centre, const juce::String& label, bool output);
    }
}
