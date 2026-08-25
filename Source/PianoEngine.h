#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

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
        const juce::String& name,
        int velocityLayer = 3);

    void setTouch(float touch);

    void noteOn(int midiNote, float velocity = 0.8f);
    void noteOff(int midiNote);

    void renderNextBlock(
        juce::AudioBuffer<float>& buffer,
        int startSample,
        int numSamples);

private:
    static constexpr int numberOfVelocityLayers = 4;

    std::array<juce::Synthesiser, numberOfVelocityLayers> synthesisers;
    juce::AudioFormatManager formatManager;

    std::atomic<float> touchTarget { 0.5f };
    std::array<int, 128> activeLayerForNote {};

    bool installReader(
        std::unique_ptr<juce::AudioFormatReader> reader,
        int rootMidiNote,
        int lowMidiNote,
        int highMidiNote,
        const juce::String& name,
        int velocityLayer);

    int chooseVelocityLayer() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoEngine)
};
