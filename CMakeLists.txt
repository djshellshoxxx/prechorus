cmake_minimum_required(VERSION 3.22)
project(PreChorus VERSION 0.0.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static MSVC runtime so the plug-ins and standalone app run without a VC++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(PRECHORUS_COPY_AFTER_BUILD "Copy plug-ins to the system plug-in folders after building" OFF)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

FetchContent_Declare(clap_juce_extensions
    GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
    GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d
    GIT_SUBMODULES_RECURSE TRUE)
FetchContent_MakeAvailable(clap_juce_extensions)

set(PRECHORUS_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PRECHORUS_FORMATS AU)
endif()

juce_add_plugin(PreChorus
    VERSION ${PROJECT_VERSION}
    COMPANY_NAME "Circuit Drift Labs"
    COMPANY_WEBSITE "https://djshellshoxxx.github.io/circuitdriftlabs/"
    BUNDLE_ID com.circuitdriftlabs.prechorus
    PLUGIN_MANUFACTURER_CODE Shdv
    PLUGIN_CODE PrCh
    FORMATS ${PRECHORUS_FORMATS}
    PRODUCT_NAME "PreChorus"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Fx Modulation
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_128.png"
    COPY_PLUGIN_AFTER_BUILD ${PRECHORUS_COPY_AFTER_BUILD})

clap_juce_extensions_plugin(TARGET PreChorus
    CLAP_ID "com.circuitdriftlabs.prechorus"
    CLAP_FEATURES audio-effect chorus stereo)

juce_generate_juce_header(PreChorus)

target_sources(PreChorus PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(PreChorus PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    PRECHORUS_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(PreChorus
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
