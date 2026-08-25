#pragma once

#include <JuceHeader.h>
#include <atomic>

class Conductor
{
public:
    struct Decision
    {
        int chordIndex = 0;
        double durationSeconds = 10.0;
        float brightness = 0.5f;
        float level = 0.5f;
    };

    void setEnergy(double newEnergy);
    Decision getNextDecision(int currentChord);

private:
    int chooseNextChord(int currentChord);
    double chooseDuration(double currentEnergy);

    std::atomic<double> energy { 0.5 };
    juce::Random random;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Conductor)
};
