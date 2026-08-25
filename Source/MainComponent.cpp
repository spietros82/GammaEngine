#include "MainComponent.h"
#include "BinaryData.h"

MainComponent::MainComponent()
{
    setSize(900, 760);

    energySlider.setRange(0.0, 100.0, 1.0);
    energySlider.setValue(50.0);
    energySlider.setTextValueSuffix(" %");
    energySlider.setSliderStyle(juce::Slider::LinearHorizontal);
    energySlider.setTextBoxStyle(
        juce::Slider::TextBoxRight, false, 80, 25);

    energySlider.onValueChange = [this]
    {
        musicEngine.setEnergy(
            energySlider.getValue() / 100.0);
    };

    energyLabel.setText(
        "Energy",
        juce::dontSendNotification);
    energyLabel.setJustificationType(
        juce::Justification::centred);

    addAndMakeVisible(energySlider);
    addAndMakeVisible(energyLabel);

    depthSlider.setRange(0.0, 100.0, 1.0);
    depthSlider.setValue(50.0);
    depthSlider.setTextValueSuffix(" %");

    depthSlider.onValueChange = [this]
    {
        gammaEngine.setModulationDepth(
            depthSlider.getValue() / 100.0);
    };

    depthLabel.setText(
        "Gamma Modulation Depth",
        juce::dontSendNotification);
    depthLabel.setJustificationType(
        juce::Justification::centred);

    addAndMakeVisible(depthSlider);
    addAndMakeVisible(depthLabel);

    gammaSlider.setRange(30.0, 50.0, 0.1);
    gammaSlider.setValue(40.0);
    gammaSlider.setTextValueSuffix(" Hz");

    gammaSlider.onValueChange = [this]
    {
        gammaEngine.setGammaFrequency(
            gammaSlider.getValue());
    };

    gammaLabel.setText(
        "Gamma Frequency",
        juce::dontSendNotification);
    gammaLabel.setJustificationType(
        juce::Justification::centred);

    addAndMakeVisible(gammaSlider);
    addAndMakeVisible(gammaLabel);

    setAudioChannels(0, 2);
    startTimerHz(30);
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

void MainComponent::prepareToPlay(
    int samplesPerBlockExpected,
    double sampleRate)
{
    displaySampleRate = sampleRate;

    gammaEngine.prepare(sampleRate);
    musicEngine.prepare(sampleRate);
    pianoEngine.prepare(sampleRate);

    pianoBuffer.setSize(
        2,
        juce::jmax(1, samplesPerBlockExpected),
        false,
        false,
        true);

    const bool pianoLoaded =
        pianoEngine.loadSampleFromMemory(
            BinaryData::GPiano_sus_C4_v4_rr1_Player_flac,
            static_cast<size_t>(
                BinaryData::GPiano_sus_C4_v4_rr1_Player_flacSize));

    if (pianoLoaded)
        pianoEngine.noteOn(60, 0.7f);

    musicEngine.setEnergy(
        energySlider.getValue() / 100.0);

    gammaEngine.setGammaFrequency(
        gammaSlider.getValue());

    gammaEngine.setModulationDepth(
        depthSlider.getValue() / 100.0);
}

void MainComponent::getNextAudioBlock(
    const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* buffer = bufferToFill.buffer;

    if (buffer == nullptr)
        return;

    buffer->clear(
        bufferToFill.startSample,
        bufferToFill.numSamples);

    if (pianoBuffer.getNumSamples() < bufferToFill.numSamples)
        pianoBuffer.setSize(
            2,
            bufferToFill.numSamples,
            false,
            false,
            true);

    pianoBuffer.clear();
    pianoEngine.renderNextBlock(
        pianoBuffer,
        0,
        bufferToFill.numSamples);

    for (int i = 0; i < bufferToFill.numSamples; ++i)
    {
        const float musicSample =
            musicEngine.getNextSample();

        float pianoSample = 0.0f;

        if (pianoBuffer.getNumChannels() >= 2)
        {
            pianoSample =
                0.5f * (
                    pianoBuffer.getSample(0, i)
                    + pianoBuffer.getSample(1, i));
        }
        else if (pianoBuffer.getNumChannels() == 1)
        {
            pianoSample =
                pianoBuffer.getSample(0, i);
        }

        const float mixedSample =
            musicSample * 0.65f
            + pianoSample * 0.50f;

        const float outputSample =
            gammaEngine.processSample(mixedSample);

        const int sampleIndex =
            bufferToFill.startSample + i;

        for (int channel = 0;
             channel < buffer->getNumChannels();
             ++channel)
        {
            buffer->setSample(
                channel,
                sampleIndex,
                outputSample);
        }

        pushSample(outputSample);
    }
}

void MainComponent::pushSample(float sample)
{
    const int numToWrite =
        sampleFifo.getFreeSpace() > 0 ? 1 : 0;

    if (numToWrite == 0)
        return;

    int start1 = 0, size1 = 0;
    int start2 = 0, size2 = 0;

    sampleFifo.prepareToWrite(
        1,
        start1,
        size1,
        start2,
        size2);

    if (size1 > 0)
        fifoStorage[static_cast<size_t>(start1)] = sample;
    else if (size2 > 0)
        fifoStorage[static_cast<size_t>(start2)] = sample;

    sampleFifo.finishedWrite(size1 + size2);
}

void MainComponent::timerCallback()
{
    const int ready = sampleFifo.getNumReady();

    if (ready <= 0)
    {
        repaint();
        return;
    }

    const int numToRead =
        juce::jmin(ready, displayBufferSize);

    int start1 = 0, size1 = 0;
    int start2 = 0, size2 = 0;

    sampleFifo.prepareToRead(
        numToRead,
        start1,
        size1,
        start2,
        size2);

    const int totalRead = size1 + size2;

    if (totalRead > 0)
    {
        if (totalRead < displayBufferSize)
        {
            std::move(
                displayBuffer.begin() + totalRead,
                displayBuffer.end(),
                displayBuffer.begin());
        }

        int destIndex =
            displayBufferSize - totalRead;

        for (int i = 0; i < size1; ++i)
            displayBuffer[static_cast<size_t>(destIndex++)] =
                fifoStorage[static_cast<size_t>(start1 + i)];

        for (int i = 0; i < size2; ++i)
            displayBuffer[static_cast<size_t>(destIndex++)] =
                fifoStorage[static_cast<size_t>(start2 + i)];
    }

    sampleFifo.finishedRead(totalRead);

    std::fill(fftData.begin(), fftData.end(), 0.0f);

    const int available =
        juce::jmin(fftSize, displayBufferSize);

    const int sourceStart =
        displayBufferSize - available;

    for (int i = 0; i < available; ++i)
        fftData[static_cast<size_t>(i)] =
            displayBuffer[static_cast<size_t>(sourceStart + i)];

    fftWindow.multiplyWithWindowingTable(
        fftData.data(),
        fftSize);

    forwardFFT.performFrequencyOnlyForwardTransform(
        fftData.data());

    for (int i = 0; i < fftSize / 2; ++i)
        spectrumData[static_cast<size_t>(i)] =
            fftData[static_cast<size_t>(i)];

    repaint();
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(
        getLookAndFeel()
            .findColour(
                juce::ResizableWindow::backgroundColourId));

    g.setColour(
        getLookAndFeel()
            .findColour(
                juce::Label::textColourId));

    g.setFont(24.0f);

    g.drawFittedText(
        "Gamma Engine",
        20,
        20,
        getWidth() - 40,
        40,
        juce::Justification::centred,
        1);

    drawOscilloscope(g);
    drawSpectrum(g);
}

void MainComponent::drawOscilloscope(
    juce::Graphics& g)
{
    if (oscilloscopeArea.isEmpty())
        return;

    g.setColour(
        getLookAndFeel()
            .findColour(
                juce::Label::textColourId)
            .withAlpha(0.25f));

    g.drawRect(oscilloscopeArea);

    juce::Path path;

    const float centreY =
        oscilloscopeArea.getCentreY();

    const float height =
        oscilloscopeArea.getHeight() * 0.42f;

    const int samplesToDraw =
        juce::jmin(
            4000,
            displayBufferSize);

    const int start =
        displayBufferSize - samplesToDraw;

    for (int i = 0; i < samplesToDraw; ++i)
    {
        const float x =
            juce::jmap(
                static_cast<float>(i),
                0.0f,
                static_cast<float>(samplesToDraw - 1),
                oscilloscopeArea.getX(),
                oscilloscopeArea.getRight());

        const float y =
            centreY -
            displayBuffer[static_cast<size_t>(start + i)]
            * height;

        if (i == 0)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    g.setColour(
        getLookAndFeel()
            .findColour(
                juce::Slider::thumbColourId));

    g.strokePath(
        path,
        juce::PathStrokeType(1.5f));

    g.setColour(
        getLookAndFeel()
            .findColour(
                juce::Label::textColourId));

    g.setFont(14.0f);

    g.drawText(
        "Oscilloscope",
        oscilloscopeArea
            .withHeight(22.0f)
            .toNearestInt(),
        juce::Justification::centredLeft);
}

void MainComponent::drawSpectrum(
    juce::Graphics& g)
{
    if (spectrumArea.isEmpty())
        return;

    g.setColour(
        getLookAndFeel()
            .findColour(
                juce::Label::textColourId)
            .withAlpha(0.25f));

    g.drawRect(spectrumArea);

    const double maxFrequency = 1000.0;
    const int maxBin =
        juce::jlimit(
            1,
            fftSize / 2 - 1,
            static_cast<int>(
                maxFrequency
                * fftSize
                / displaySampleRate));

    juce::Path path;

    for (int bin = 1; bin <= maxBin; ++bin)
    {
        const float magnitude =
            spectrumData[static_cast<size_t>(bin)];

        const float db =
            juce::Decibels::gainToDecibels(
                magnitude / static_cast<float>(fftSize),
                -100.0f);

        const float x =
            juce::jmap(
                static_cast<float>(bin),
                1.0f,
                static_cast<float>(maxBin),
                spectrumArea.getX(),
                spectrumArea.getRight());

        const float y =
            juce::jmap(
                db,
                -100.0f,
                0.0f,
                spectrumArea.getBottom(),
                spectrumArea.getY());

        if (bin == 1)
            path.startNewSubPath(x, y);
        else
            path.lineTo(x, y);
    }

    g.setColour(
        getLookAndFeel()
            .findColour(
                juce::Slider::thumbColourId));

    g.strokePath(
        path,
        juce::PathStrokeType(1.5f));

    g.setColour(
        getLookAndFeel()
            .findColour(
                juce::Label::textColourId));

    g.setFont(14.0f);

    g.drawText(
        "FFT Spectrum 0–1000 Hz",
        spectrumArea
            .withHeight(22.0f)
            .toNearestInt(),
        juce::Justification::centredLeft);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    area.removeFromTop(100);

    const int sideMargin = 60;
    area.reduce(sideMargin, 0);

    const int controlHeight = 40;
    const int labelHeight = 25;
    const int gap = 15;

    energyLabel.setBounds(
        area.removeFromTop(labelHeight));

    energySlider.setBounds(
        area.removeFromTop(controlHeight));

    area.removeFromTop(gap);

    depthLabel.setBounds(
        area.removeFromTop(labelHeight));

    depthSlider.setBounds(
        area.removeFromTop(controlHeight));

    area.removeFromTop(gap);

    gammaLabel.setBounds(
        area.removeFromTop(labelHeight));

    gammaSlider.setBounds(
        area.removeFromTop(controlHeight));

    area.removeFromTop(30);

    const int graphGap = 30;

    const int availableHeight =
        juce::jmax(
            0,
            area.getHeight() - graphGap);

    const int scopeHeight =
        availableHeight / 2;

    oscilloscopeArea =
        area.removeFromTop(scopeHeight)
            .toFloat();

    area.removeFromTop(graphGap);

    spectrumArea =
        area.toFloat();
}
