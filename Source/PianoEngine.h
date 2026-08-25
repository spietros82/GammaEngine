#pragma once

#include <JuceHeader.h>

class PianoEngine
{
public:
    PianoEngine() = default;

    void prepare(double sampleRate);
    bool loadSample(const juce::File& sampleFile);

    void noteOn(int midiNote, float velocity = 0.8f);
    void noteOff(int midiNote);

    void renderNextBlock(
        juce::AudioBuffer<float>& buffer,
        int startSample,
        int numSamples);

private:
    juce::Synthesiser synthesiser;
    juce::AudioFormatManager formatManager;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoEngine)
};
