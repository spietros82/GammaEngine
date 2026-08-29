#include "PianoPerformer.h"

#include <algorithm>
#include <cmath>

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
    samplesUntilNextGesture = 0;
    lastMelodicNote = -1;
    lastGesture = Gesture::Chord;
    consecutiveRests = 0;

    gestureHistory.fill(Gesture::Rest);
    gestureHistoryCount = 0;

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

    // A harmonic change is itself a performance opportunity, but after this
    // the pianist runs on an independent clock while the chord can remain held.
    beginGesture(true);
    scheduleNextGestureTime();
}

void PianoPerformer::processBlock(int numSamples)
{
    if (numSamples <= 0 || currentChord < 0)
        return;

    samplesUntilNextGesture -= numSamples;

    if (samplesUntilNextGesture <= 0)
    {
        stopActiveNotes();
        clearSchedule();
        beginGesture(false);
        scheduleNextGestureTime();
    }

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

void PianoPerformer::beginGesture(bool harmonicChange)
{
    auto gesture = chooseGesture();

    // On a fresh harmony, avoid opening with too many empty events.
    if (harmonicChange && gesture == Gesture::Rest && random.nextFloat() < 0.65f)
        gesture = energy < 0.35f ? Gesture::Single : Gesture::Dyad;

    const auto notes = chooseNotesForGesture(gesture);
    scheduleGesture(gesture, notes);

    lastGesture = gesture;
    rememberGesture(gesture);

    if (gesture == Gesture::Rest)
        ++consecutiveRests;
    else
        consecutiveRests = 0;

    advancePhrase();
}

void PianoPerformer::scheduleNextGestureTime()
{
    // This is independent from harmonic duration. Low energy breathes for longer;
    // high energy creates several possible piano events within one chord.
    const double baseSeconds = juce::jmap(
        static_cast<double>(energy),
        0.0,
        1.0,
        7.5,
        1.7);

    const double variationSeconds = juce::jmap(
        static_cast<double>(energy),
        0.0,
        1.0,
        3.0,
        1.8);

    double seconds = baseSeconds + random.nextDouble() * variationSeconds;

    // Phrase peaks become slightly more active without becoming metronomic.
    seconds *= juce::jmap(
        static_cast<double>(getPhraseIntensity()),
        0.0,
        1.0,
        1.15,
        0.82);

    samplesUntilNextGesture = juce::jmax(
        1,
        static_cast<int>(std::round(seconds * currentSampleRate)));
}

PianoPerformer::Gesture PianoPerformer::chooseGesture()
{
    float restWeight = juce::jmap(energy, 0.0f, 1.0f, 0.30f, 0.05f);
    float singleWeight = juce::jmap(energy, 0.0f, 1.0f, 0.34f, 0.12f);
    float dyadWeight = juce::jmap(energy, 0.0f, 1.0f, 0.22f, 0.23f);
    float chordWeight = juce::jmap(energy, 0.0f, 1.0f, 0.10f, 0.25f);
    float arpeggioWeight = juce::jmap(energy, 0.0f, 1.0f, 0.04f, 0.35f);

    const float phraseIntensity = getPhraseIntensity();

    restWeight *= juce::jmap(phraseIntensity, 0.0f, 1.0f, 1.20f, 0.55f);
    singleWeight *= juce::jmap(phraseIntensity, 0.0f, 1.0f, 1.15f, 0.85f);
    arpeggioWeight *= juce::jmap(phraseIntensity, 0.0f, 1.0f, 0.70f, 1.35f);

    if (lastGesture == Gesture::Rest)
        restWeight *= 0.05f;
    if (lastGesture == Gesture::Single)
        singleWeight *= 0.62f;
    if (lastGesture == Gesture::Dyad)
        dyadWeight *= 0.68f;
    if (lastGesture == Gesture::Chord)
        chordWeight *= 0.65f;
    if (lastGesture == Gesture::Arpeggio)
        arpeggioWeight *= 0.48f;

    if (countRecentGesture(Gesture::Arpeggio, 4) >= 2)
        arpeggioWeight *= 0.35f;
    if (countRecentGesture(Gesture::Chord, 4) >= 2)
        chordWeight *= 0.45f;
    if (countRecentGesture(Gesture::Single, 4) >= 2)
        singleWeight *= 0.55f;

    if (consecutiveRests >= 1)
        restWeight = 0.0f;

    const float total =
        restWeight + singleWeight + dyadWeight + chordWeight + arpeggioWeight;

    const float roll = random.nextFloat() * total;
    float threshold = restWeight;

    if (roll < threshold)
        return Gesture::Rest;

    threshold += singleWeight;
    if (roll < threshold)
        return Gesture::Single;

    threshold += dyadWeight;
    if (roll < threshold)
        return Gesture::Dyad;

    threshold += chordWeight;
    if (roll < threshold)
        return Gesture::Chord;

    return Gesture::Arpeggio;
}

std::array<int, 3> PianoPerformer::chooseNotesForGesture(Gesture gesture)
{
    std::array<int, 3> notes { 60, 64, 67 };

    if (gesture == Gesture::Rest)
        return notes;

    if (gesture == Gesture::Single)
    {
        const bool preferChord = random.nextFloat() < 0.58f;
        notes[0] = choosePaletteNote(preferChord, lastMelodicNote);
        notes[1] = notes[0];
        notes[2] = notes[0];
        lastMelodicNote = notes[0];
        return notes;
    }

    if (gesture == Gesture::Dyad)
    {
        notes[0] = choosePaletteNote(true, lastMelodicNote);
        notes[1] = choosePaletteNote(random.nextFloat() < 0.55f, notes[0]);

        if (notes[1] < notes[0])
            std::swap(notes[0], notes[1]);

        if (notes[1] - notes[0] > 12)
            notes[1] -= 12;

        notes[2] = notes[1];
        lastMelodicNote = notes[1];
        return notes;
    }

    // Chords and arpeggios still honour the harmony, but no longer use only two
    // hard-coded shapes. We build three notes from the current palette and allow
    // one colour tone when the gesture is an arpeggio.
    notes[0] = choosePaletteNote(true, lastMelodicNote);
    notes[1] = choosePaletteNote(true, notes[0]);
    notes[2] = choosePaletteNote(
        gesture == Gesture::Chord || random.nextFloat() < 0.68f,
        notes[1]);

    std::sort(notes.begin(), notes.end());

    for (size_t i = 1; i < notes.size(); ++i)
    {
        while (notes[i] <= notes[i - 1])
            notes[i] += 12;
    }

    // Keep the generated shape inside a comfortable piano register.
    while (notes[2] > 81)
    {
        for (auto& note : notes)
            note -= 12;
    }

    if (energy > 0.72f
        && getPhraseIntensity() > 0.55f
        && random.nextFloat() < 0.22f)
    {
        notes[2] += 12;
    }

    lastMelodicNote = notes[2];
    return notes;
}

int PianoPerformer::choosePaletteNote(bool preferChordTone, int avoidNote)
{
    constexpr int lowMidi = 45;   // A2
    constexpr int highMidi = 76;  // E5

    std::array<int, 40> candidates {};
    int count = 0;

    for (int midi = lowMidi; midi <= highMidi; ++midi)
    {
        const bool acceptable = preferChordTone
            ? isChordTone(midi)
            : isScaleTone(midi);

        if (! acceptable)
            continue;

        if (midi == avoidNote && random.nextFloat() < 0.85f)
            continue;

        candidates[static_cast<size_t>(count++)] = midi;
    }

    if (count <= 0)
        return 60;

    int chosen = candidates[static_cast<size_t>(random.nextInt(count))];

    // Phrase contour gently biases register rather than forcing a fixed voicing.
    const float intensity = getPhraseIntensity();

    if (phraseShape == PhraseShape::Rise && intensity > 0.55f && chosen < 60)
        chosen += 12;
    else if (phraseShape == PhraseShape::Fall && intensity < 0.45f && chosen > 64)
        chosen -= 12;
    else if (phraseShape == PhraseShape::Arch && intensity > 0.60f && chosen < 57)
        chosen += 12;

    return juce::jlimit(lowMidi, 81, chosen);
}

bool PianoPerformer::isChordTone(int midiNote) const
{
    const int pc = ((midiNote % 12) + 12) % 12;

    switch (currentChord)
    {
        case 0: // Am: A C E
            return pc == 9 || pc == 0 || pc == 4;
        case 1: // F: F A C
            return pc == 5 || pc == 9 || pc == 0;
        case 2: // C: C E G
            return pc == 0 || pc == 4 || pc == 7;
        case 3: // G: G B D
            return pc == 7 || pc == 11 || pc == 2;
        default:
            return pc == 0 || pc == 4 || pc == 7;
    }
}

bool PianoPerformer::isScaleTone(int midiNote) const
{
    // A natural minor / C major pitch collection: A B C D E F G.
    const int pc = ((midiNote % 12) + 12) % 12;
    return pc == 9 || pc == 11 || pc == 0 || pc == 2
        || pc == 4 || pc == 5 || pc == 7;
}

void PianoPerformer::scheduleGesture(
    Gesture gesture,
    const std::array<int, 3>& notes)
{
    if (gesture == Gesture::Rest)
        return;

    if (gesture == Gesture::Single)
    {
        scheduledNotes[0] = {
            notes[0],
            0,
            makeVelocity(),
            true
        };
        return;
    }

    if (gesture == Gesture::Dyad)
    {
        const double spreadMs = 35.0 + random.nextDouble() * 180.0;

        scheduledNotes[0] = { notes[0], 0, makeVelocity(), true };
        scheduledNotes[1] = {
            notes[1],
            millisecondsToSamples(spreadMs),
            makeVelocity(),
            true
        };
        return;
    }

    if (gesture == Gesture::Chord)
    {
        const double maxSpreadMs = juce::jmap(
            static_cast<double>(energy),
            0.0,
            1.0,
            220.0,
            45.0);

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

    const double minStepMs = juce::jmap(
        static_cast<double>(energy),
        0.0,
        1.0,
        330.0,
        80.0);

    const double variationMs = juce::jmap(
        static_cast<double>(energy),
        0.0,
        1.0,
        280.0,
        150.0);

    const double stepMs = minStepMs + random.nextDouble() * variationMs;

    bool descending = random.nextBool();

    if (phraseShape == PhraseShape::Rise)
        descending = random.nextFloat() < 0.18f;
    else if (phraseShape == PhraseShape::Fall)
        descending = random.nextFloat() < 0.76f;

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
    phraseLength = 5 + random.nextInt(4); // 5 to 8 performance gestures.

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
        gestureHistory[static_cast<size_t>(i)] =
            gestureHistory[static_cast<size_t>(i - 1)];

    gestureHistory[0] = gesture;
    gestureHistoryCount = juce::jmin(
        gestureHistoryCount + 1,
        gestureHistorySize);
}

int PianoPerformer::countRecentGesture(Gesture gesture, int lookBack) const
{
    const int count = juce::jmin(
        lookBack,
        juce::jmin(gestureHistoryCount, gestureHistorySize));

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
    const float base = 0.27f + energy * 0.20f + phraseIntensity * 0.08f;
    const float humanVariation = (random.nextFloat() - 0.5f) * 0.10f;

    return juce::jlimit(0.18f, 0.66f, base + humanVariation);
}
