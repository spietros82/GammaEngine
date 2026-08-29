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

    Gesture lastGesture = Gesture::Chord;
    int consecutiveRests = 0;

    std::array<int, 4> activeNotes { -1, -1, -1, -1 };
    std::array<ScheduledNote, 4> scheduledNotes {};

    Gesture chooseGesture();
    std::array<int, 3> chooseVoicing(int chordIndex);
    void scheduleGesture(Gesture gesture, const std::array<int, 3>& notes);
    void stopActiveNotes();
    void clearSchedule();
    void triggerNote(int midiNote, float velocity);

    int millisecondsToSamples(double milliseconds) const;
    float makeVelocity() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoPerformer)
};
