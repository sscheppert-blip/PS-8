cmake_minimum_required(VERSION 3.22)

project(PS8 VERSION 0.5.0 LANGUAGES C CXX)

option(PS8_BUILD_TESTS "Build the offline tests" ON)
option(PS8_COPY_PLUGIN "Copy the built plugins into the user's plugin folders" ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Dependencies are fetched into the build tree, never committed.
include(FetchContent)

FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        9.0.3
    GIT_SHALLOW    TRUE
    EXCLUDE_FROM_ALL)

FetchContent_Declare(clap_juce_extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG        7adee3a1bd4684d4caa5601100e364abccff4b4d
    EXCLUDE_FROM_ALL)

FetchContent_MakeAvailable(JUCE clap_juce_extensions)

# ---------------------------------------------------------------------------
# Plugin
# ---------------------------------------------------------------------------
juce_add_plugin(PS8
    PRODUCT_NAME             "PS-8"
    COMPANY_NAME             "sscheppert-blip"
    COMPANY_WEBSITE          "https://github.com/sscheppert-blip"
    BUNDLE_ID                "io.github.sscheppert-blip.ps8"
    PLUGIN_MANUFACTURER_CODE Sbli
    PLUGIN_CODE              Ps08
    ICON_BIG                 "${CMAKE_CURRENT_SOURCE_DIR}/design/icon.png"
    FORMATS                  AU VST3 Standalone
    IS_SYNTH                 TRUE
    NEEDS_MIDI_INPUT         TRUE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD  ${PS8_COPY_PLUGIN}
    VST3_CATEGORIES          Instrument Drum Synth
    AU_MAIN_TYPE             kAudioUnitType_MusicDevice
    DESCRIPTION              "Analog-style percussion synthesizer with step sequencer and patchbay")

set(PS8_SOURCES
    Source/dsp/Engine.cpp
    Source/dsp/Sequencer.cpp
    Source/FactoryPresets.cpp
    Source/Parameters.cpp
    Source/Patterns.cpp
    Source/Presets.cpp
    Source/PluginProcessor.cpp
    Source/StandaloneApp.cpp
    Source/PluginEditor.cpp
    Source/ui/Patchbay.cpp
    Source/ui/ProgramDisplay.cpp
    Source/ui/SequencerTools.cpp
    Source/ui/ServiceBoard.cpp
    Source/ui/StatusDisplay.cpp
    Source/ui/Theme.cpp
    Source/ui/Widgets.cpp)

target_sources(PS8 PRIVATE ${PS8_SOURCES})

# Saira Stencil One (SIL Open Font License, see design/fonts/OFL.txt) for titles and the logo
juce_add_binary_data(PS8Fonts
    HEADER_NAME PS8Fonts.h
    NAMESPACE   PS8Fonts
    SOURCES     design/fonts/SairaStencilOne-Regular.ttf)

target_include_directories(PS8 PRIVATE Source)

target_compile_definitions(PS8
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)   # Source/StandaloneApp.cpp: native title bar

target_link_libraries(PS8
    PRIVATE
        PS8Fonts
        clap_juce_extensions
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

clap_juce_extensions_plugin(TARGET PS8
    CLAP_ID       "io.github.sscheppert-blip.ps8"
    CLAP_FEATURES instrument drum synthesizer mono)

# JUCE writes moduleinfo.json into the VST3 after signing, and the CLAP and AU bundles are
# not signed at all; Apple Silicon hosts refuse to load bundles without a valid signature.
if(APPLE)
    # The copy into ~/Library/Audio/Plug-Ins happens before this step, so the
    # installed copy is re-signed as well.
    foreach(format VST3 CLAP AU)
        set(folder ${format})
        if(format STREQUAL "AU")
            set(folder Components)
        endif()
        set(installed "$ENV{HOME}/Library/Audio/Plug-Ins/${folder}/$<TARGET_BUNDLE_DIR_NAME:PS8_${format}>")
        add_custom_command(TARGET PS8_${format} POST_BUILD
            COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:PS8_${format}>"
            COMMAND test ! -d "${installed}" || codesign --force --deep --sign - "${installed}"
            VERBATIM)
    endforeach()

    # The linker signs only the Standalone binary, not its resources (icon), so sign the whole app;
    # with PS8_COPY_PLUGIN the signed app also replaces ~/Applications/PS-8.app.
    add_custom_command(TARGET PS8_Standalone POST_BUILD
        COMMAND codesign --force --deep --sign - "$<TARGET_BUNDLE_DIR:PS8_Standalone>"
        VERBATIM)
    if(PS8_COPY_PLUGIN)
        set(installedApp "$ENV{HOME}/Applications/$<TARGET_BUNDLE_DIR_NAME:PS8_Standalone>")
        add_custom_command(TARGET PS8_Standalone POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf "${installedApp}"
            COMMAND ditto "$<TARGET_BUNDLE_DIR:PS8_Standalone>" "${installedApp}"
            COMMAND ${CMAKE_COMMAND} -E echo "Installed ${installedApp}"
            VERBATIM)
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if(PS8_BUILD_TESTS)
    juce_add_console_app(PS8Tests PRODUCT_NAME "PS8Tests")

    target_sources(PS8Tests PRIVATE ${PS8_SOURCES} tests/Tests.cpp)
    target_include_directories(PS8Tests PRIVATE Source)
    target_compile_definitions(PS8Tests
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_MODAL_LOOPS_PERMITTED=1
            JucePlugin_Name="PS-8")

    target_link_libraries(PS8Tests
        PRIVATE
            PS8Fonts
            clap_juce_extensions
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    enable_testing()
    add_test(NAME PS8Tests COMMAND PS8Tests)
endif()
