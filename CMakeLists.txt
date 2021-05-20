cmake_minimum_required(VERSION 3.12)
project(SpringViz VERSION 0.0.1)

add_subdirectory(libs/JUCE)                    # If you've put JUCE in a subdirectory called JUCE

juce_add_gui_app(spring-viz
    PRODUCT_NAME "SpringVisualization")     # The name of the final executable, which can differ from the target name

juce_generate_juce_header(spring-viz)

juce_add_binary_data( spring-viz-binary
        SOURCES resources/GlacialIndifference-Regular.ttf)

target_sources(spring-viz
    PRIVATE
        src/Spring.cpp
        src/SpringComponent.cpp)

target_compile_definitions(spring-viz
    PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_gui_app` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_gui_app` call
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:spring-viz,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:spring-viz,JUCE_VERSION>")


target_link_libraries(spring-viz
    PRIVATE
        # GuiAppData            # If we'd created a binary data target, we'd link to it here
        juce::juce_gui_extra
        juce::juce_audio_formats
        juce::juce_audio_devices
        juce::juce_audio_utils
        juce::juce_audio_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        spring-viz-binary
        )

target_compile_options(spring-viz PUBLIC -Wno-implicit-float-conversion)
