cmake_minimum_required(VERSION 3.22)
project(LeadSynth VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_plugin(LeadSynth
    COMPANY_NAME "LeadSynth"
    PLUGIN_MANUFACTURER_CODE Lead
    PLUGIN_CODE Ldvs
    FORMATS Standalone VST3 AU
    PRODUCT_NAME "LeadSynth"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    ICON_BIG "${CMAKE_SOURCE_DIR}/Assets/icon_1024.png"
    ICON_SMALL "${CMAKE_SOURCE_DIR}/Assets/icon_1024.png"
)

target_sources(LeadSynth
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/SynthVoice.cpp
        Source/StandaloneApp.cpp
)

target_compile_definitions(LeadSynth
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
)

# Replace JUCE's single-resolution icon with our proper multi-resolution icns
add_custom_command(TARGET LeadSynth_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/Assets/LeadSynth.icns"
        "$<TARGET_BUNDLE_DIR:LeadSynth_Standalone>/Contents/Resources/Icon.icns"
    COMMENT "Installing multi-resolution app icon"
)

target_link_libraries(LeadSynth
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
