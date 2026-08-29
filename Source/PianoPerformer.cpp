#include "PianoPerformer.h"

PianoPerformer::PianoPerformer(PianoEngine& engine)
    : pianoEngine(engine),
      random(static_cast<int64>(juce::Time::getMillisecondCounterHiRes()))
{
}

void PianoPerformer::prepare(double sampleRate)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 48000.0;
    reset();
}

void PianoPerformer::reset()
{
    stopActiveNotes();
    clearSchedule();
    currentChord = -1;
    lastGesture = Gesture::Chord;
    consecutiveRests = 0;
}

void PianoPerformer::setEnergy(float newEnergy)
{
    energy = juce::jlimit(0.0f, 1.0f, newEnergy);
}

void PianoPerformer::onChordChanged(int chordIndex)
{
    if (chordIndex == currentChord)
        return;

    currentChord = chordIndex;

    stopActiveNotes();
    clearSchedule();

    const auto gesture = chooseGesture();
    const auto notes = chooseVoicing(chordIndex);

    scheduleGesture(gesture, notes);

    lastGesture = gesture;

    if (gesture == Gesture::Rest)
        ++consecutiveRests;
    else
        consecutiveRests = 0;
}

void PianoPerformer::processBlock(int numSamples)
{
    if (numSamples <= 0)
        return;

    for (auto& note : scheduledNotes)
    {
        if (! note.pending)
            continue;

        if (note.samplesUntilTrigger <= numSamples)
        {
            triggerNote(note.midiNote, note.velocity);
            note.pending = false;
        }
        else
        {
            note.samplesUntilTrigger -= numSamples;
        }
    }
}

PianoPerformer::Gesture PianoPerformer::chooseGesture()
{
    // Energy changes the performance style rather than merely the loudness.
    // Low energy leaves more space. High energy favours motion.
    float restWeight = juce::jmap(energy, 0.0f, 1.0f, 0.34f, 0.05f);
    float chordWeight = juce::jmap(energy, 0.0f, 1.0f, 0.46f, 0.32f);
    float arpeggioWeight = juce::jmap(energy, 0.0f, 1.0f, 0.20f, 0.63f);

    // Phrase memory: avoid immediately repeating the same obvious gesture.
    if (lastGesture == Gesture::Rest)
        restWeight *= 0.08f;

    if (lastGesture == Gesture::Arpeggio)
        arpeggioWeight *= 0.62f;

    if (lastGesture == Gesture::Chord)
        chordWeight *= 0.78f;

    // Never allow the performer to vanish for too long.
    if (consecutiveRests >= 1)
        restWeight = 0.0f;

    const float total = restWeight + chordWeight + arpeggioWeight;
    const float roll = random.nextFloat() * total;

    if (roll < restWeight)
        return Gesture::Rest;

    if (roll < restWeight + chordWeight)
        return Gesture::Chord;

    return Gesture::Arpeggio;
}

std::array<int, 3> PianoPerformer::chooseVoicing(int chordIndex)
{
    const bool alternate = random.nextBool();
    const bool wider = energy > 0.62f && random.nextFloat() < 0.35f;

    std::array<int, 3> notes { 60, 64, 67 };

    switch (chordIndex)
    {
        case 0: // Am
            notes = alternate
                ? std::array<int, 3>{ 60, 64, 69 }
                : std::array<int, 3>{ 57, 60, 64 };
            break;

        case 1: // F
            notes = alternate
                ? std::array<int, 3>{ 57, 60, 65 }
                : std::array<int, 3>{ 53, 57, 60 };
            break;

        case 2: // C
            notes = alternate
                ? std::array<int, 3>{ 55, 60, 64 }
                : std::array<int, 3>{ 60, 64, 67 };
            break;

        case 3: // G
            notes = alternate
                ? std::array<int, 3>{ 59, 62, 67 }
                : std::array<int, 3>{ 55, 59, 62 };
            break;

        default:
            break;
    }

    if (wider)
        notes[2] += 12;

    return notes;
}

void PianoPerformer::scheduleGesture(
    Gesture gesture,
    const std::array<int, 3>& notes)
{
    if (gesture == Gesture::Rest)
        return;

    if (gesture == Gesture::Chord)
    {
        // A chord is not necessarily mechanically simultaneous.
        const double maxSpreadMs = juce::jmap(
            static_cast<double>(energy),
            0.0,
            1.0,
            180.0,
            35.0);

        const double spreadMs = random.nextDouble() * maxSpreadMs;

        for (size_t i = 0; i < notes.size(); ++i)
        {
            scheduledNotes[i] = {
                notes[i],
                millisecondsToSamples(spreadMs * static_cast<double>(i)),
                makeVelocity(),
                true
            };
        }

        return;
    }

    // Arpeggio spacing gets tighter as Energy rises.
    const double minStepMs = juce::jmap(
        static_cast<double>(energy),
        0.0,
        1.0,
        260.0,
        70.0);

    const double variationMs = juce::jmap(
        static_cast<double>(energy),
        0.0,
        1.0,
        260.0,
        110.0);

    const double stepMs = minStepMs + random.nextDouble() * variationMs;
    const bool descending = random.nextFloat() < 0.28f;

    for (size_t i = 0; i < notes.size(); ++i)
    {
        const size_t sourceIndex = descending
            ? notes.size() - 1 - i
            : i;

        scheduledNotes[i] = {
            notes[sourceIndex],
            millisecondsToSamples(stepMs * static_cast<double>(i)),
            makeVelocity(),
            true
        };
    }
}

void PianoPerformer::stopActiveNotes()
{
    for (auto& note : activeNotes)
    {
        if (note >= 0)
            pianoEngine.noteOff(note);

        note = -1;
    }
}

void PianoPerformer::clearSchedule()
{
    for (auto& note : scheduledNotes)
        note = {};
}

void PianoPerformer::triggerNote(int midiNote, float velocity)
{
    pianoEngine.noteOn(midiNote, velocity);

    for (auto& active : activeNotes)
    {
        if (active < 0)
        {
            active = midiNote;
            return;
        }
    }

    // Defensive fallback if the active-note list is unexpectedly full.
    pianoEngine.noteOff(activeNotes[0]);
    activeNotes[0] = midiNote;
}

int PianoPerformer::millisecondsToSamples(double milliseconds) const
{
    return static_cast<int>(
        std::round(milliseconds * 0.001 * currentSampleRate));
}

float PianoPerformer::makeVelocity()
{
    const float base = 0.30f + energy * 0.22f;
    const float humanVariation = (random.nextFloat() - 0.5f) * 0.08f;

    return juce::jlimit(0.20f, 0.62f, base + humanVariation);
}
