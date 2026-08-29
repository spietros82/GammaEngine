#pragma once

#include <JuceHeader.h>
#include <array>
#include "PianoEngine.h"

class PianoPerformer
{
public:
    enum class Gesture
    {
        Rest,
        Single,
        Dyad,
        Chord,
        Arpeggio
    };

    explicit PianoPerformer(PianoEngine& engine);

    void prepare(double sampleRate);
    void reset();
    void setEnergy(float newEnergy);

    void onChordChanged(int chordIndex);
    void processBlock(int numSamples);

    Gesture getLastGesture() const noexcept { return lastGesture; }

private:
    enum class PhraseShape
    {
        Rise,
        Fall,
        Arch
    };

    struct ScheduledNote
    {
        int midiNote = -1;
        int samplesUntilTrigger = 0;
        float velocity = 0.4f;
        bool pending = false;
    };

    PianoEngine& pianoEngine;
    juce::Random random;

    double currentSampleRate = 48000.0;
    float energy = 0.5f;
    int currentChord = -1;
    int samplesUntilNextGesture = 0;
    int lastMelodicNote = -1;

    Gesture lastGesture = Gesture::Chord;
    int consecutiveRests = 0;

    static constexpr int gestureHistorySize = 8;
    std::array<Gesture, gestureHistorySize> gestureHistory {};
    int gestureHistoryCount = 0;

    int phrasePosition = 0;
    int phraseLength = 6;
    PhraseShape phraseShape = PhraseShape::Arch;

    std::array<int, 6> activeNotes { -1, -1, -1, -1, -1, -1 };
    std::array<ScheduledNote, 6> scheduledNotes {};

    void beginGesture(bool harmonicChange);
    void scheduleNextGestureTime();
    Gesture chooseGesture();
    std::array<int, 3> chooseNotesForGesture(Gesture gesture);
    int choosePaletteNote(bool preferChordTone, int avoidNote = -1);
    bool isChordTone(int midiNote) const;
    bool isScaleTone(int midiNote) const;

    void scheduleGesture(Gesture gesture, const std::array<int, 3>& notes);
    void stopActiveNotes();
    void clearSchedule();
    void triggerNote(int midiNote, float velocity);

    void startNewPhrase();
    void advancePhrase();
    void rememberGesture(Gesture gesture);
    int countRecentGesture(Gesture gesture, int lookBack) const;
    float getPhraseIntensity() const;

    int millisecondsToSamples(double milliseconds) const;
    float makeVelocity();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoPerformer)
};
