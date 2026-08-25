#pragma once

#include <JuceHeader.h>
#include <atomic>

class GammaEngine
{
public:
    GammaEngine() = default;

    void prepare(double sampleRate);
    void setGammaFrequency(double frequency);
    void setModulationDepth(double depth);

    float processSample(float inputSample);

private:
    double currentSampleRate = 48000.0;
    double modulatorPhase = 0.0;

    std::atomic<double> gammaFrequency { 40.0 };
    std::atomic<double> modulationDepth { 0.5 };

    juce::SmoothedValue<double> smoothedGammaFrequency;
    juce::SmoothedValue<double> smoothedModulationDepth;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GammaEngine)
};
