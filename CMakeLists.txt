cmake_minimum_required(VERSION 3.22)
project(AdlibSakura VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Descarga JUCE automáticamente (no hay que instalar nada a mano)
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AdlibSakura
    COMPANY_NAME "TuNombre"                 # <- cámbialo por tu nombre artístico
    PLUGIN_MANUFACTURER_CODE Tnom           # 4 letras, la primera en mayúscula
    PLUGIN_CODE Adsk                        # 4 letras, único para este plugin
    FORMATS VST3 Standalone
    PRODUCT_NAME "Adlib Sakura"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx)

target_sources(AdlibSakura PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Sakura.h)

target_compile_definitions(AdlibSakura PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AdlibSakura
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
