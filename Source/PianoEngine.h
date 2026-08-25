#pragma once

#include <JuceHeader.h>

class PianoEngine
{
public:
    PianoEngine() = default;

    void prepare(double sampleRate);

    void clearSamples();

    bool loadSample(const juce::File& sampleFile);
    bool loadSampleFromMemory(const void* data, size_t dataSize);

    bool addSampleFromMemory(
        const void* data,
        size_t dataSize,
        int rootMidiNote,
        int lowMidiNote,
        int highMidiNote,
        const juce::String& name);

    void noteOn(int midiNote, float velocity = 0.8f);
    void noteOff(int midiNote);

    void renderNextBlock(
        juce::AudioBuffer<float>& buffer,
        int startSample,
        int numSamples);

private:
    juce::Synthesiser synthesiser;
    juce::AudioFormatManager formatManager;

    bool installReader(
        std::unique_ptr<juce::AudioFormatReader> reader,
        int rootMidiNote,
        int lowMidiNote,
        int highMidiNote,
        const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoEngine)
};
