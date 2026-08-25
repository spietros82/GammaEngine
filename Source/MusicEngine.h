#pragma once

#include <JuceHeader.h>
#include <array>
#include "Conductor.h"

class MusicEngine
{
public:
    MusicEngine() = default;

    void prepare(double sampleRate);
    float getNextSample();
    void setEnergy(double energy);

    int getCurrentChord() const noexcept
    {
        return currentChord;
    }

private:
    void advanceChord();

    double currentSampleRate = 48000.0;

    double phase1a = 0.0;
    double phase1b = 0.0;
    double phase2a = 0.0;
    double phase2b = 0.0;
    double phase3a = 0.0;
    double phase3b = 0.0;

    double filterLfoPhase = 0.0;
    double amplitudeLfoPhase = 0.0;

    juce::SmoothedValue<double> frequency1;
    juce::SmoothedValue<double> frequency2;
    juce::SmoothedValue<double> frequency3;

    juce::SmoothedValue<float> adaptiveBrightness;
    juce::SmoothedValue<float> adaptiveLevel;

    juce::dsp::StateVariableTPTFilter<float> lowPassFilter;

    static constexpr int numberOfChords = 4;

    const std::array<std::array<double, 3>, numberOfChords> chordFrequencies
    {{
        { 220.0000, 261.6256, 329.6276 }, // Am
        { 174.6141, 220.0000, 261.6256 }, // F
        { 261.6256, 329.6276, 392.0000 }, // C
        { 196.0000, 246.9417, 293.6648 }  // G
    }};

    int currentChord = 0;
    int samplesUntilChordChange = 0;
    float level = 0.08f;

    Conductor conductor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MusicEngine)
};
