#include "PianoEngine.h"

void PianoEngine::prepare(double sampleRate)
{
    formatManager.registerBasicFormats();

    synthesiser.clearVoices();

    for (int i = 0; i < 8; ++i)
        synthesiser.addVoice(new juce::SamplerVoice());

    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

void PianoEngine::clearSamples()
{
    synthesiser.clearSounds();
}

bool PianoEngine::installReader(
    std::unique_ptr<juce::AudioFormatReader> reader,
    int rootMidiNote,
    int lowMidiNote,
    int highMidiNote,
    const juce::String& name)
{
    if (reader == nullptr)
        return false;

    juce::BigInteger notes;
    notes.setRange(
        lowMidiNote,
        highMidiNote - lowMidiNote + 1,
        true);

    synthesiser.addSound(
        new juce::SamplerSound(
            name,
            *reader,
            notes,
            rootMidiNote,
            0.01,
            0.5,
            10.0));

    return true;
}

bool PianoEngine::loadSample(const juce::File& sampleFile)
{
    clearSamples();

    return installReader(
        std::unique_ptr<juce::AudioFormatReader>(
            formatManager.createReaderFor(sampleFile)),
        60,
        0,
        127,
        "Piano");
}

bool PianoEngine::loadSampleFromMemory(
    const void* data,
    size_t dataSize)
{
    clearSamples();

    return addSampleFromMemory(
        data,
        dataSize,
        60,
        0,
        127,
        "Piano");
}

bool PianoEngine::addSampleFromMemory(
    const void* data,
    size_t dataSize,
    int rootMidiNote,
    int lowMidiNote,
    int highMidiNote,
    const juce::String& name)
{
    auto stream = std::make_unique<juce::MemoryInputStream>(
        data,
        dataSize,
        false);

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(std::move(stream)));

    return installReader(
        std::move(reader),
        rootMidiNote,
        lowMidiNote,
        highMidiNote,
        name);
}

void PianoEngine::noteOn(int midiNote, float velocity)
{
    synthesiser.noteOn(1, midiNote, velocity);
}

void PianoEngine::noteOff(int midiNote)
{
    synthesiser.noteOff(1, midiNote, 0.0f, true);
}

void PianoEngine::renderNextBlock(
    juce::AudioBuffer<float>& buffer,
    int startSample,
    int numSamples)
{
    juce::MidiBuffer emptyMidi;
    synthesiser.renderNextBlock(
        buffer,
        emptyMidi,
        startSample,
        numSamples);
}
