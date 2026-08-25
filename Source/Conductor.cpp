#include "Conductor.h"

void Conductor::setEnergy(double newEnergy)
{
    energy.store(juce::jlimit(0.0, 1.0, newEnergy));
}

Conductor::Decision Conductor::getNextDecision(int currentChord)
{
    Decision decision;
    const double currentEnergy = energy.load();

    decision.chordIndex = chooseNextChord(currentChord);
    decision.durationSeconds = chooseDuration(currentEnergy);
    decision.brightness = static_cast<float>(0.25 + currentEnergy * 0.75);
    decision.level = static_cast<float>(0.65 + currentEnergy * 0.35);

    return decision;
}

int Conductor::chooseNextChord(int currentChord)
{
    const int roll = random.nextInt(100);

    switch (currentChord)
    {
        case 0: // Am
            if (roll < 45) return 1;
            if (roll < 75) return 2;
            return 3;

        case 1: // F
            if (roll < 60) return 2;
            return 0;

        case 2: // C
            if (roll < 50) return 3;
            if (roll < 80) return 0;
            return 1;

        case 3: // G
            if (roll < 70) return 0;
            return 2;

        default:
            return 0;
    }
}

double Conductor::chooseDuration(double currentEnergy)
{
    const double minimumDuration = 12.0 - currentEnergy * 6.0;
    const double variation = 6.0 - currentEnergy * 2.0;

    return minimumDuration + random.nextDouble() * variation;
}
