cmake_minimum_required(VERSION 3.22)
project(PreChorus VERSION 1.4.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PRECHORUS_BUILD_CLAP "Also build the CLAP format via clap-juce-extensions" ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# AU only exists on Apple; asking for it elsewhere just makes noise.
set(PRECHORUS_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PRECHORUS_FORMATS AU)
endif()

juce_add_plugin(PreChorus
    COMPANY_NAME "SheldonDavidson"
    PLUGIN_MANUFACTURER_CODE Shdv
    PLUGIN_CODE PrCh
    FORMATS ${PRECHORUS_FORMATS}
    PRODUCT_NAME "PreChorus"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_1024.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon_128.png"
    COPY_PLUGIN_AFTER_BUILD TRUE)

juce_generate_juce_header(PreChorus)

target_sources(PreChorus PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Theme.cpp
    Source/Controls.cpp
    Source/Pages.cpp
    Source/Colony.cpp
    Source/Talkbox.cpp)

target_compile_definitions(PreChorus PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(PreChorus
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# --- CLAP -------------------------------------------------------------------
# clap-juce-extensions wraps the existing JUCE target, so the CLAP build shares
# every source file, parameter and bit of state with the VST3.
if(PRECHORUS_BUILD_CLAP)
    FetchContent_Declare(clap-juce-extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG main
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(clap-juce-extensions)

    clap_juce_extensions_plugin(TARGET PreChorus
        CLAP_ID "com.sheldondavidson.prechorus"
        CLAP_FEATURES audio-effect chorus stereo
        CLAP_MANUAL_URL "https://github.com/djshellshoxxx/prechorus"
        CLAP_SUPPORT_URL "https://github.com/djshellshoxxx/prechorus/issues")
endif()
