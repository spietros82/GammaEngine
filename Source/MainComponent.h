#pragma once

#include <JuceHeader.h>
#include <array>
#include "GammaEngine.h"
#include "MusicEngine.h"
#include "PianoEngine.h"

class MainComponent final
    : public juce::AudioAppComponent,
      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override {}

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void pushSample(float sample);
    void drawOscilloscope(juce::Graphics& g);
    void drawSpectrum(juce::Graphics& g);

    GammaEngine gammaEngine;
    MusicEngine musicEngine;
    PianoEngine pianoEngine;
    juce::AudioBuffer<float> pianoBuffer;

    juce::Slider energySlider;
    juce::Label energyLabel;

    juce::Slider depthSlider;
    juce::Label depthLabel;

    juce::Slider gammaSlider;
    juce::Label gammaLabel;

    double displaySampleRate = 48000.0;

    static constexpr int fifoSize = 32768;
    std::array<float, fifoSize> fifoStorage {};
    juce::AbstractFifo sampleFifo { fifoSize };

    static constexpr int displayBufferSize = 12000;
    std::array<float, displayBufferSize> displayBuffer {};

    juce::Rectangle<float> oscilloscopeArea;
    juce::Rectangle<float> spectrumArea;

    static constexpr int fftOrder = 13;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT forwardFFT { fftOrder };
    juce::dsp::WindowingFunction<float> fftWindow {
        fftSize,
        juce::dsp::WindowingFunction<float>::hann
    };

    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize / 2> spectrumData {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
