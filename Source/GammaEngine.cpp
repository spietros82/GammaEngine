#include "GammaEngine.h"

void GammaEngine::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    modulatorPhase = 0.0;

    constexpr double smoothingTime = 0.05;

    smoothedGammaFrequency.reset(currentSampleRate, smoothingTime);
    smoothedModulationDepth.reset(currentSampleRate, smoothingTime);

    smoothedGammaFrequency.setCurrentAndTargetValue(gammaFrequency.load());
    smoothedModulationDepth.setCurrentAndTargetValue(modulationDepth.load());
}

void GammaEngine::setGammaFrequency(double frequency)
{
    gammaFrequency.store(frequency);
}

void GammaEngine::setModulationDepth(double depth)
{
    modulationDepth.store(juce::jlimit(0.0, 1.0, depth));
}

float GammaEngine::processSample(float inputSample)
{
    smoothedGammaFrequency.setTargetValue(gammaFrequency.load());
    smoothedModulationDepth.setTargetValue(modulationDepth.load());

    const double gammaHz = smoothedGammaFrequency.getNextValue();
    const double depth = smoothedModulationDepth.getNextValue();

    const double gammaWave = std::sin(modulatorPhase);

    const double modulation =
        1.0 - (depth * 0.5) + (depth * 0.5 * gammaWave);

    modulatorPhase +=
        juce::MathConstants<double>::twoPi * gammaHz / currentSampleRate;

    if (modulatorPhase >= juce::MathConstants<double>::twoPi)
        modulatorPhase -= juce::MathConstants<double>::twoPi;

    return static_cast<float>(inputSample * modulation);
}
