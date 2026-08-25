#include "PianoEngine.h"

void PianoEngine::prepare(double sampleRate)
{
    formatManager.registerBasicFormats();

    for (auto& synthesiser : synthesisers)
    {
        synthesiser.clearVoices();

        for (int i = 0; i < 8; ++i)
            synthesiser.addVoice(new juce::SamplerVoice());

        synthesiser.setCurrentPlaybackSampleRate(sampleRate);
    }

    activeLayerForNote.fill(-1);
}

void PianoEngine::clearSamples()
{
    for (auto& synthesiser : synthesisers)
        synthesiser.clearSounds();

    activeLayerForNote.fill(-1);
}

bool PianoEngine::installReader(
    std::unique_ptr<juce::AudioFormatReader> reader,
    int rootMidiNote,
    int lowMidiNote,
    int highMidiNote,
    const juce::String& name,
    int velocityLayer)
{
    if (reader == nullptr)
        return false;

    const int layer = juce::jlimit(
        0,
        numberOfVelocityLayers - 1,
        velocityLayer);

    juce::BigInteger notes;
    notes.setRange(
        lowMidiNote,
        highMidiNote - lowMidiNote + 1,
        true);

    synthesisers[static_cast<size_t>(layer)].addSound(
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
        "Piano",
        3);
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
        "Piano",
        3);
}

bool PianoEngine::addSampleFromMemory(
    const void* data,
    size_t dataSize,
    int rootMidiNote,
    int lowMidiNote,
    int highMidiNote,
    const juce::String& name,
    int velocityLayer)
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
        name,
        velocityLayer);
}

void PianoEngine::setTouch(float touch)
{
    touchTarget.store(
        juce::jlimit(0.0f, 1.0f, touch),
        std::memory_order_relaxed);
}

int PianoEngine::chooseVelocityLayer() const
{
    const float touch = touchTarget.load(std::memory_order_relaxed);

    if (touch < 0.25f)
        return 0;

    if (touch < 0.50f)
        return 1;

    if (touch < 0.75f)
        return 2;

    return 3;
}

void PianoEngine::noteOn(int midiNote, float velocity)
{
    if (midiNote < 0 || midiNote >= 128)
        return;

    const int layer = chooseVelocityLayer();

    // The source recordings get progressively louder across v1..v4.
    // These factors reduce that raw level jump so layer selection is heard
    // more as a change in touch/timbre than as a simple volume control.
    constexpr std::array<float, numberOfVelocityLayers> layerCompensation
    {
        1.00f,
        0.90f,
        0.80f,
        0.72f
    };

    const float compensatedVelocity =
        juce::jlimit(
            0.0f,
            1.0f,
            velocity * layerCompensation[static_cast<size_t>(layer)]);

    synthesisers[static_cast<size_t>(layer)].noteOn(
        1,
        midiNote,
        compensatedVelocity);

    activeLayerForNote[static_cast<size_t>(midiNote)] = layer;
}

void PianoEngine::noteOff(int midiNote)
{
    if (midiNote < 0 || midiNote >= 128)
        return;

    const int layer = activeLayerForNote[static_cast<size_t>(midiNote)];

    if (layer >= 0 && layer < numberOfVelocityLayers)
    {
        synthesisers[static_cast<size_t>(layer)].noteOff(
            1,
            midiNote,
            0.0f,
            true);
    }
    else
    {
        for (auto& synthesiser : synthesisers)
            synthesiser.noteOff(1, midiNote, 0.0f, true);
    }

    activeLayerForNote[static_cast<size_t>(midiNote)] = -1;
}

void PianoEngine::renderNextBlock(
    juce::AudioBuffer<float>& buffer,
    int startSample,
    int numSamples)
{
    juce::MidiBuffer emptyMidi;

    for (auto& synthesiser : synthesisers)
    {
        synthesiser.renderNextBlock(
            buffer,
            emptyMidi,
            startSample,
            numSamples);
    }
}
