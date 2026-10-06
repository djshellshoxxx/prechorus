cmake_minimum_required(VERSION 3.22)
project(PreChorus VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(PreChorus
    COMPANY_NAME "SheldonDavidson"
    PLUGIN_MANUFACTURER_CODE Shdv
    PLUGIN_CODE PrCh
    FORMATS VST3 AU Standalone
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
    Source/PluginEditor.cpp)

target_compile_definitions(PreChorus PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(PreChorus
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)


option(PRECHORUS_BUILD_TESTS "Build PreChorus regression tests" ON)
if(PRECHORUS_BUILD_TESTS)
    enable_testing()
    juce_add_console_app(PreChorusTests PRODUCT_NAME "PreChorus Tests")
    juce_generate_juce_header(PreChorusTests)
    target_sources(PreChorusTests PRIVATE
        Tests/PreChorusTests.cpp
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp)
    target_compile_definitions(PreChorusTests PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JucePlugin_Name="PreChorus")
    target_link_libraries(PreChorusTests
        PRIVATE juce::juce_audio_utils
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
    add_test(NAME PreChorusCoreTests COMMAND PreChorusTests)
endif()
