#include "MusicEngine.h"

void MusicEngine::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;

    phase1a = phase1b = 0.0;
    phase2a = phase2b = 0.0;
    phase3a = phase3b = 0.0;

    filterLfoPhase = 0.0;
    amplitudeLfoPhase = 0.0;
    currentChord = 0;

    constexpr double glideTimeSeconds = 3.0;

    frequency1.reset(currentSampleRate, glideTimeSeconds);
    frequency2.reset(currentSampleRate, glideTimeSeconds);
    frequency3.reset(currentSampleRate, glideTimeSeconds);

    frequency1.setCurrentAndTargetValue(chordFrequencies[0][0]);
    frequency2.setCurrentAndTargetValue(chordFrequencies[0][1]);
    frequency3.setCurrentAndTargetValue(chordFrequencies[0][2]);

    constexpr double adaptiveSmoothingSeconds = 4.0;

    adaptiveBrightness.reset(currentSampleRate, adaptiveSmoothingSeconds);
    adaptiveLevel.reset(currentSampleRate, adaptiveSmoothingSeconds);

    adaptiveBrightness.setCurrentAndTargetValue(0.5f);
    adaptiveLevel.setCurrentAndTargetValue(0.8f);

    samplesUntilChordChange =
        static_cast<int>(currentSampleRate * 10.0);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;

    lowPassFilter.prepare(spec);
    lowPassFilter.reset();
    lowPassFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lowPassFilter.setCutoffFrequency(1200.0f);
    lowPassFilter.setResonance(0.7f);
}

void MusicEngine::setEnergy(double energy)
{
    conductor.setEnergy(energy);
}

void MusicEngine::advanceChord()
{
    const auto decision = conductor.getNextDecision(currentChord);

    currentChord = decision.chordIndex;

    frequency1.setTargetValue(chordFrequencies[currentChord][0]);
    frequency2.setTargetValue(chordFrequencies[currentChord][1]);
    frequency3.setTargetValue(chordFrequencies[currentChord][2]);

    adaptiveBrightness.setTargetValue(decision.brightness);
    adaptiveLevel.setTargetValue(decision.level);

    samplesUntilChordChange =
        static_cast<int>(currentSampleRate * decision.durationSeconds);
}

float MusicEngine::getNextSample()
{
    if (--samplesUntilChordChange <= 0)
        advanceChord();

    const double freq1 = frequency1.getNextValue();
    const double freq2 = frequency2.getNextValue();
    const double freq3 = frequency3.getNextValue();

    constexpr double detune = 0.003;

    const double sample1 =
        (std::sin(phase1a) + std::sin(phase1b)) * 0.5;

    const double sample2 =
        (std::sin(phase2a) + std::sin(phase2b)) * 0.5;

    const double sample3 =
        (std::sin(phase3a) + std::sin(phase3b)) * 0.5;

    const float chord =
        static_cast<float>((sample1 + sample2 + sample3) / 3.0);

    constexpr double filterLfoHz = 0.05;
    constexpr double amplitudeLfoHz = 0.033;

    const double filterLfo =
        0.5 + 0.5 * std::sin(filterLfoPhase);

    const double amplitudeLfo =
        0.5 + 0.5 * std::sin(amplitudeLfoPhase);

    const float brightness = adaptiveBrightness.getNextValue();

    const float cutoff =
        static_cast<float>(
            500.0 + brightness * 1300.0 + filterLfo * 400.0);

    lowPassFilter.setCutoffFrequency(cutoff);

    const float filtered =
        lowPassFilter.processSample(0, chord);

    phase1a += juce::MathConstants<double>::twoPi
        * freq1 / currentSampleRate;
    phase1b += juce::MathConstants<double>::twoPi
        * freq1 * (1.0 + detune) / currentSampleRate;

    phase2a += juce::MathConstants<double>::twoPi
        * freq2 / currentSampleRate;
    phase2b += juce::MathConstants<double>::twoPi
        * freq2 * (1.0 - detune) / currentSampleRate;

    phase3a += juce::MathConstants<double>::twoPi
        * freq3 / currentSampleRate;
    phase3b += juce::MathConstants<double>::twoPi
        * freq3 * (1.0 + detune) / currentSampleRate;

    filterLfoPhase += juce::MathConstants<double>::twoPi
        * filterLfoHz / currentSampleRate;

    amplitudeLfoPhase += juce::MathConstants<double>::twoPi
        * amplitudeLfoHz / currentSampleRate;

    auto wrapPhase = [](double& phase)
    {
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
    };

    wrapPhase(phase1a);
    wrapPhase(phase1b);
    wrapPhase(phase2a);
    wrapPhase(phase2b);
    wrapPhase(phase3a);
    wrapPhase(phase3b);
    wrapPhase(filterLfoPhase);
    wrapPhase(amplitudeLfoPhase);

    const float breathing =
        static_cast<float>(0.75 + amplitudeLfo * 0.25);

    const float adaptiveGain =
        adaptiveLevel.getNextValue();

    return filtered * level * breathing * adaptiveGain;
}
