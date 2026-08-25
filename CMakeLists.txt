cmake_minimum_required(VERSION 3.22)

project(GammaEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 9.0.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(GammaEngine
    PRODUCT_NAME "Gamma Engine"
    COMPANY_NAME "Gamma Engine"
    BUNDLE_ID "com.gammaengine.app"
)

juce_generate_juce_header(GammaEngine)

target_sources(GammaEngine
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/MainComponent.h
        Source/GammaEngine.cpp
        Source/GammaEngine.h
        Source/MusicEngine.cpp
        Source/MusicEngine.h
        Source/Conductor.cpp
        Source/Conductor.h
        Source/PianoEngine.cpp
        Source/PianoEngine.h
)

target_compile_definitions(GammaEngine
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:GammaEngine,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:GammaEngine,JUCE_VERSION>"
)

target_link_libraries(GammaEngine
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
