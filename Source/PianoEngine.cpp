#include "PianoEngine.h"

void PianoEngine::prepare(double sampleRate)
{
    formatManager.registerBasicFormats();

    synthesiser.clearVoices();

    for (int i = 0; i < 8; ++i)
        synthesiser.addVoice(new juce::SamplerVoice());

    synthesiser.setCurrentPlaybackSampleRate(sampleRate);
}

bool PianoEngine::loadSample(const juce::File& sampleFile)
{
    synthesiser.clearSounds();

    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(sampleFile));

    if (reader == nullptr)
        return false;

    juce::BigInteger notes;
    notes.setRange(0, 128, true);

    constexpr int rootMidiNote = 60;

    synthesiser.addSound(
        new juce::SamplerSound(
            "Piano",
            *reader,
            notes,
            rootMidiNote,
            0.01,
            0.5,
            10.0));

    return true;
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
