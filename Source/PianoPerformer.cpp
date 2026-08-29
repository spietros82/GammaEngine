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
    gestureHistory.fill(Gesture::Rest);
    gestureHistoryCount = 0;
    hasPreviousVoicing = false;
    startNewPhrase();
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
    rememberGesture(gesture);

    if (gesture == Gesture::Rest)
        ++consecutiveRests;
    else
        consecutiveRests = 0;

    previousVoicing = notes;
    hasPreviousVoicing = true;
    advancePhrase();
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
    float restWeight = juce::jmap(energy, 0.0f, 1.0f, 0.34f, 0.05f);
    float chordWeight = juce::jmap(energy, 0.0f, 1.0f, 0.46f, 0.32f);
    float arpeggioWeight = juce::jmap(energy, 0.0f, 1.0f, 0.20f, 0.63f);

    const float phraseIntensity = getPhraseIntensity();

    // The phrase shape biases motion over several harmonic changes.
    arpeggioWeight *= juce::jmap(phraseIntensity, 0.0f, 1.0f, 0.72f, 1.35f);
    chordWeight *= juce::jmap(phraseIntensity, 0.0f, 1.0f, 1.18f, 0.88f);
    restWeight *= juce::jmap(phraseIntensity, 0.0f, 1.0f, 1.25f, 0.55f);

    // Phrase openings tend to establish space. Endings tend to resolve.
    if (phrasePosition == 0)
    {
        chordWeight *= 1.18f;
        restWeight *= 1.10f;
        arpeggioWeight *= 0.72f;
    }

    if (phrasePosition == phraseLength - 1)
    {
        chordWeight *= 1.55f;
        arpeggioWeight *= 0.60f;
        restWeight *= 0.48f;
    }

    // Avoid obvious local repetition.
    if (lastGesture == Gesture::Rest)
        restWeight *= 0.08f;

    if (lastGesture == Gesture::Arpeggio)
        arpeggioWeight *= 0.62f;

    if (lastGesture == Gesture::Chord)
        chordWeight *= 0.78f;

    // Memory over several gestures: too many of one type pushes the performer away from it.
    if (countRecentGesture(Gesture::Arpeggio, 3) >= 2)
        arpeggioWeight *= 0.45f;

    if (countRecentGesture(Gesture::Chord, 3) >= 2)
        chordWeight *= 0.60f;

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
    std::array<int, 3> lowVoicing { 60, 64, 67 };
    std::array<int, 3> highVoicing { 60, 64, 67 };

    switch (chordIndex)
    {
        case 0: // Am
            lowVoicing = { 57, 60, 64 };
            highVoicing = { 60, 64, 69 };
            break;

        case 1: // F
            lowVoicing = { 53, 57, 60 };
            highVoicing = { 57, 60, 65 };
            break;

        case 2: // C
            lowVoicing = { 55, 60, 64 };
            highVoicing = { 60, 64, 67 };
            break;

        case 3: // G
            lowVoicing = { 55, 59, 62 };
            highVoicing = { 59, 62, 67 };
            break;

        default:
            break;
    }

    const float phraseIntensity = getPhraseIntensity();
    bool useHigh = false;

    // Phrase direction nudges register, giving the ear a contour to follow.
    switch (phraseShape)
    {
        case PhraseShape::Rise:
            useHigh = random.nextFloat() < juce::jmap(phraseIntensity, 0.0f, 1.0f, 0.25f, 0.88f);
            break;

        case PhraseShape::Fall:
            useHigh = random.nextFloat() < juce::jmap(phraseIntensity, 0.0f, 1.0f, 0.82f, 0.22f);
            break;

        case PhraseShape::Arch:
            useHigh = random.nextFloat() < juce::jmap(phraseIntensity, 0.0f, 1.0f, 0.30f, 0.82f);
            break;
    }

    auto notes = useHigh ? highVoicing : lowVoicing;

    // Avoid an identical register choice if the previous voicing already had the same top note.
    if (hasPreviousVoicing && notes[2] == previousVoicing[2] && random.nextFloat() < 0.62f)
        notes = useHigh ? lowVoicing : highVoicing;

    // At energetic phrase peaks, occasionally widen the top voice by an octave.
    const bool wider =
        energy > 0.62f
        && phraseIntensity > 0.58f
        && random.nextFloat() < 0.28f;

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

    const float phraseIntensity = getPhraseIntensity();

    if (gesture == Gesture::Chord)
    {
        const double baseSpreadMs = juce::jmap(
            static_cast<double>(energy),
            0.0,
            1.0,
            180.0,
            35.0);

        const double phraseFactor = juce::jmap(
            static_cast<double>(phraseIntensity),
            0.0,
            1.0,
            1.25,
            0.72);

        const double spreadMs = random.nextDouble() * baseSpreadMs * phraseFactor;

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

    const double phraseSpeed = juce::jmap(
        static_cast<double>(phraseIntensity),
        0.0,
        1.0,
        1.18,
        0.78);

    const double stepMs =
        (minStepMs + random.nextDouble() * variationMs) * phraseSpeed;

    bool descending = random.nextFloat() < 0.28f;

    if (phraseShape == PhraseShape::Rise)
        descending = random.nextFloat() < 0.12f;
    else if (phraseShape == PhraseShape::Fall)
        descending = random.nextFloat() < 0.72f;

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

    pianoEngine.noteOff(activeNotes[0]);
    activeNotes[0] = midiNote;
}

void PianoPerformer::startNewPhrase()
{
    phrasePosition = 0;
    phraseLength = 4 + random.nextInt(3); // 4, 5 or 6 harmonic changes.

    switch (random.nextInt(3))
    {
        case 0:
            phraseShape = PhraseShape::Rise;
            break;
        case 1:
            phraseShape = PhraseShape::Fall;
            break;
        default:
            phraseShape = PhraseShape::Arch;
            break;
    }
}

void PianoPerformer::advancePhrase()
{
    ++phrasePosition;

    if (phrasePosition >= phraseLength)
        startNewPhrase();
}

void PianoPerformer::rememberGesture(Gesture gesture)
{
    for (int i = gestureHistorySize - 1; i > 0; --i)
        gestureHistory[static_cast<size_t>(i)] = gestureHistory[static_cast<size_t>(i - 1)];

    gestureHistory[0] = gesture;
    gestureHistoryCount = juce::jmin(gestureHistoryCount + 1, gestureHistorySize);
}

int PianoPerformer::countRecentGesture(Gesture gesture, int lookBack) const
{
    const int count = juce::jmin({ lookBack, gestureHistoryCount, gestureHistorySize });
    int matches = 0;

    for (int i = 0; i < count; ++i)
    {
        if (gestureHistory[static_cast<size_t>(i)] == gesture)
            ++matches;
    }

    return matches;
}

float PianoPerformer::getPhraseIntensity() const
{
    if (phraseLength <= 1)
        return 0.5f;

    const float position = juce::jlimit(
        0.0f,
        1.0f,
        static_cast<float>(phrasePosition)
            / static_cast<float>(phraseLength - 1));

    switch (phraseShape)
    {
        case PhraseShape::Rise:
            return position;

        case PhraseShape::Fall:
            return 1.0f - position;

        case PhraseShape::Arch:
            return 1.0f - std::abs(position * 2.0f - 1.0f);
    }

    return 0.5f;
}

int PianoPerformer::millisecondsToSamples(double milliseconds) const
{
    return static_cast<int>(
        std::round(milliseconds * 0.001 * currentSampleRate));
}

float PianoPerformer::makeVelocity()
{
    const float phraseIntensity = getPhraseIntensity();
    const float base = 0.28f + energy * 0.20f + phraseIntensity * 0.07f;
    const float humanVariation = (random.nextFloat() - 0.5f) * 0.08f;

    return juce::jlimit(0.20f, 0.64f, base + humanVariation);
}
