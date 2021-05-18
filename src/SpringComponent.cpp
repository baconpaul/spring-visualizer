#include "SpringComponent.h"

//==============================================================================
SpringComponent::SpringComponent() : forwardFFT(fftOrder)
{
    setSize(1280, 720);

    setAudioChannels(0, 2);

    formatManager.registerBasicFormats();

    auto dir = std::string("/Users/paul/dev/music/spring-visualizer/resources/data/");
    auto file = juce::File(dir + "Audio.wav");          // [9]
    auto *reader = formatManager.createReaderFor(file); // [10]

    if (reader != nullptr)
    {
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource = std::move(newSource);
        transportSource.setPosition(0.0);
        priorTime = 0.0;
    }
    startTimerHz(30);

    auto epf = juce::File(dir + "EP.mid");
    auto fis = std::make_unique<juce::FileInputStream>(epf);
    epFile.readFrom(*fis);
    epFile.convertTimestampTicksToSeconds();

    auto ppf = juce::File(dir + "Pad.mid");
    auto fps = std::make_unique<juce::FileInputStream>(ppf);
    padFile.readFrom(*fps);
    padFile.convertTimestampTicksToSeconds();

    memset(fftData, 0, sizeof(float) * fftSize);

    addKeyListener(this);
}

SpringComponent::~SpringComponent() { shutdownAudio(); }

//==============================================================================
void SpringComponent::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)

    auto cp = transportSource.getCurrentPosition();
    if( ! transportSource.isPlaying() || cp < 10 )
    {
        g.fillAll(juce::Colour(0, 0, 0));
        g.setFont(juce::Font(50.0f));
        int c = 255;
        if( cp > 5 )
            c = 255 - ( cp - 5 ) * 255.0 / 5.0;
        if( c < 0 ) c = 0;
        g.setColour(juce::Colour(c,c,c));
        g.drawText("Title Card", getLocalBounds(), juce::Justification::centred, true);
        g.setFont(juce::Font(16.0f));
        if( transportSource.isPlaying())
            g.drawText(std::to_string(cp), getLocalBounds(), juce::Justification::bottomLeft, true);
        else
            g.drawText("Any key to start", getLocalBounds(), juce::Justification::bottomLeft, true);

        priorTime = cp;
        if( cp < 4 )
            return;
    }

    g.fillAll(juce::Colour(0, 0, 0));

    g.setFont(juce::Font(16.0f));
    g.setColour(juce::Colours::white);
    auto p = std::to_string(cp);
    g.drawText(p, getLocalBounds(), juce::Justification::bottomLeft, true);

    auto cx = getWidth() / 2;
    auto cy = getHeight() / 2;
    auto wx = 200;
    auto wy = 100;

    auto tk = epFile.getTrack(0);
    auto it = tk->getNextIndexAtTime(priorTime);
    while (it < tk->getNumEvents() & tk->getEventTime(it) <= cp)
    {
        auto m = tk->getEventPointer(it);
        if (m->message.isNoteOn())
        {
            dot d{};
            d.x = rand() % (2 * wx) - wx + cx;
            d.y = rand() % (2 * wy) - wy + cy;
            d.a = 1;
            d.square = false;

            dots.push_back(d);
        }
        it++;
    }

    tk = padFile.getTrack(0);
    it = tk->getNextIndexAtTime(priorTime);
    while (it < tk->getNumEvents() & tk->getEventTime(it) <= cp)
    {
        auto m = tk->getEventPointer(it);
        if (m->message.isNoteOn())
        {
            dot d{};
            d.x = rand() % (2 * wx) - wx + cx;
            d.y = rand() % (2 * wy) - wy + cy;
            d.a = 1;
            d.square = true;

            dots.push_back(d);
        }
        it++;
    }

    if (!dots.empty())
    {
        auto pd = dots[0];
        for (auto &d : dots)
        {
            auto c = (uint8_t)(d.a * 255);
            auto r = 30.0 / (0.2 + d.a);
            auto px = d.x;
            auto py = d.y;

            px += (1.0 - d.a) * (px - cx);
            py += (1.0 - d.a) * (py - cy);

            if (!d.square)
            {
                g.setColour(juce::Colour(c, c, 0));
                g.drawLine(px, py, pd.x, pd.y);
                g.fillEllipse(px - 0.5 * r, py - 0.5 * r, r, r);
            }
            else
            {
                g.setColour(juce::Colour(0, c, c));
                g.drawLine(px, py, pd.x, pd.y);
                g.fillRect((float)(px - 0.5 * r), (float)(py - 0.5 * r), (float)r, (float)r);
            }
            d.a *= 0.99;
            pd = d;
        }
    }

    g.setColour(juce::Colour(0, 255, 255));
    int pos = 0;
    for (auto gl : fftOut)
    {
        g.drawLine(pos, 0, pos, gl * getHeight());
        pos++;
    }
    priorTime = cp;
}

void SpringComponent::resized()
{
    // This is called when the SpringComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    toFront(true);
}

void SpringComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void SpringComponent::releaseResources() { transportSource.releaseResources(); }

void SpringComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &b)
{
    if (readerSource.get() == nullptr)
    {
        b.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(b);

    if (b.buffer->getNumChannels() == 2)
    {
        const auto *channelDataL = b.buffer->getReadPointer(0, b.startSample);
        const auto *channelDataR = b.buffer->getReadPointer(1, b.startSample);

        for (auto i = 0; i < b.numSamples; ++i)
            pushNextSampleIntoFifo(channelDataL[i]);
    }
}

void SpringComponent::timerCallback()
{
    if (nextFFTBlockReady)
    {
        forwardFFT.performFrequencyOnlyForwardTransform(fftData);

        // find the range of values produced, so we can scale our rendering to
        // show up the detail clearly
        auto maxLevel = FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);

        for (auto y = 1; y < fftSize; ++y)
        {
            auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)fftSize) * 0.2f);
            auto fftDataIndex = jlimit(0, fftSize / 2, (int)(skewedProportionY * (int)fftSize / 2));
            auto level =
                jmap(fftData[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);
            // spectrogramImage.setPixelAt (rightHandEdge, y, Colour::fromHSV (level, 1.0f,
            // level, 1.0f));
            fftOut[y - 1] = level * 0.05 + fftOut[y - 1] * 0.95;
        }
        nextFFTBlockReady = false;
    }
    repaint();
}
bool SpringComponent::keyPressed(const KeyPress &key, Component *originatingComponent)
{
    std::cout << "KeyPressed " << key.getKeyCode() << std::endl;
    if( ! transportSource.isPlaying() )
        transportSource.start();

    return true;
}
void SpringComponent::mouseUp(const MouseEvent &event) {
    if( !transportSource.isPlaying())
        transportSource.start();
    else
        transportSource.stop();
}
