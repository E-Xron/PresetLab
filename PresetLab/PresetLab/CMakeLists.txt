cmake_minimum_required(VERSION 3.22)
project(PresetLab VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
option(COPY_AFTER_BUILD "Copy the plug-ins to the system folders after building" ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4)
FetchContent_MakeAvailable(JUCE)

set(PRESETLAB_SOURCES
    Source/Dsp.h Source/Params.h Source/Instrument.h Source/Instrument.cpp Source/Effects.h Source/ChordEngine.h Source/Presets.h Source/Tuner.h
    Source/PluginProcessor.h Source/PluginProcessor.cpp Source/Look.h Source/Widgets.h Source/Pages.h Source/Panels.h
    Source/PluginEditor.h Source/PluginEditor.cpp Source/StandaloneApp.cpp)

# PresetLab: the instrument (MIDI tracks) and the PresetLab app
juce_add_plugin(PresetLab
    COMPANY_NAME "PresetLab"
    PLUGIN_MANUFACTURER_CODE Yrnm
    PLUGIN_CODE Prlb
    FORMATS VST3 AU Standalone                 # AU (Logic, GarageBand) is built on macOS only
    PRODUCT_NAME "PresetLab"
    ICON_BIG Resources/icon.png
    BUNDLE_ID com.retunelab.presetlab
    IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "PresetLab uses your audio input for the Live Input mode (guitar, bass or mic)."
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

# PresetLab FX: the same plug-in as an audio effect, for live guitar/bass/mic on audio tracks
juce_add_plugin(PresetLabFX
    COMPANY_NAME "PresetLab"
    PLUGIN_MANUFACTURER_CODE Yrnm
    PLUGIN_CODE Prfx
    FORMATS VST3 AU
    PRODUCT_NAME "PresetLab FX"
    ICON_BIG Resources/icon.png
    BUNDLE_ID com.retunelab.presetlabfx
    IS_SYNTH FALSE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Fx
    AU_MAIN_TYPE kAudioUnitType_MusicEffect
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

foreach(t PresetLab PresetLabFX)
    target_sources(${t} PRIVATE ${PRESETLAB_SOURCES})
    target_compile_definitions(${t} PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(${t} PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
endforeach()
target_compile_definitions(PresetLab PUBLIC PRESETLAB_FX=0 JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
target_compile_definitions(PresetLabFX PUBLIC PRESETLAB_FX=1)
