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
        priorTime = transportSource.getCurrentPosition();
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
    memset(fftOut, 0, sizeof(float) * fftSize);

    addKeyListener(this);
    grabKeyboardFocus();

    for (int i = 0; i < meshSize; i++)
        for (int j = 0; j < meshSize; j++)
        {
            mesh[i][j] = 0;
            meshPrior[i][j] = 0;
            lx[i][j] = 0;
            ly[i][j] = 0;
            ly2[i][j] = 0;
        }
    srand(1234); // be predictable
}

SpringComponent::~SpringComponent() { shutdownAudio(); }

void SpringComponent::paint(juce::Graphics &g)
{
    g.drawImageAt(offscreen, 0, 0);
}
//==============================================================================
void SpringComponent::paintForReal(juce::Graphics &g)
{
    auto cp = priorTime;

    g.setColour(juce::Colour(20, 30, 20));
    auto cx = getWidth() / 2;
    auto cy = getHeight() / 2;

    int pos = 0;
    int step = 10;
    for (auto gl : fftOut)
    {
        if (pos > cx)
            break;
        float npos = pos * 1.f / cx;

        g.setColour(
            juce::Colour(20 + (1 + npos) * 40 * gl, 20 + (3 - npos) * 20 * gl, 40 + 100 * gl));
        // g.setColour(juce::Colour(1.f * pos / cx * 255, 0, 0 ));
        float r = cx - pos;
        g.fillRect(r - step, 0.f, 1.f * step, 1.f * getHeight());
        r = cx + pos;
        g.fillRect(r, 0.f, 1.f * step, 1.f * getHeight());
        pos += step;
    }

    if (cp < 10)
    {
        g.setFont(juce::Font(50.0f));
        uint8_t c = 255;
        if (cp > 5)
            c = 255 - (cp - 5) * 255.0 / 5.0;
        if (c < 0)
            c = 0;
        g.setColour(juce::Colour((uint8_t)255, (uint8_t)255, (uint8_t)255, (uint8_t)c));
        g.drawText("Title Card", getLocalBounds(), juce::Justification::centred, true);
        g.setFont(juce::Font(16.0f));
        if (transportSource.isPlaying())
            g.drawText(std::to_string(cp), getLocalBounds(), juce::Justification::bottomLeft, true);
        else
            g.drawText("Any key to start", getLocalBounds(), juce::Justification::bottomLeft, true);

        priorTime = cp;
        if (cp < 3)
            return;
    }

    g.setFont(juce::Font(16.0f));
    g.setColour(juce::Colours::white);
    auto p = std::to_string(cp) + " / " + std::to_string(dots.size());
    g.drawText(p, getLocalBounds(), juce::Justification::bottomLeft, true);

    float sz = 4, sz2 = 2;
    uint8_t alpha = ( cp > 5 ? 255 : cp < 3 ? 0 : (int)( ( cp - 3 ) * 0.5 * 255 ));
    for (int j = 1; j < meshSize - 2; ++j)
    {
        g.setColour(juce::Colour( (uint8_t)(220 - j * 2), (uint8_t)(220 - j * 2),
                                  (uint8_t)(250 - j * 2), alpha ));
        float thick = 3.0 - j * 2.0 / meshSize;

        for (int i = 1; i < meshSize - 1; ++i)
        {
            g.drawLine(lx[i][j], ly2[i][j], lx[i][j + 1], ly2[i][j + 1], thick);
        }
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
                g.setColour(juce::Colour(c, c, 0, c));
                g.drawLine(px, py, pd.x, pd.y);
                g.fillEllipse(px - 0.5 * r, py - 0.5 * r, r, r);
            }
            else
            {
                g.setColour(juce::Colour(0, c, c, c));
                g.drawLine(px, py, pd.x, pd.y);
                g.fillRect((float)(px - 0.5 * r), (float)(py - 0.5 * r), (float)r, (float)r);
            }
            d.a *= 0.99;
            pd = d;
        }
    }
}

void SpringComponent::resized()
{
    // This is called when the SpringComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    offscreen = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), false);
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
        auto mk = std::max(maxLevel.getEnd(), 0.1f);
        for (auto y = 0; y < fftSize; ++y)
        {
            auto level = fftData[y] / mk;
            // fftOut[y] = level;

            fftOut[y] = level * 0.3 + fftOut[y] * 0.7;
        }
        nextFFTBlockReady = false;
    }

    auto cp = transportSource.getCurrentPosition();

    if (priorTime > 1)
    {
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

        dots.erase(std::remove_if(dots.begin(), dots.end(),
                                  [](const dot &d) { return d.a < 1.f / 255.f; }),
                   dots.end());
    }

    // this is the pad file
    auto cx = getWidth() / 2;
    auto cy = getHeight() / 2;
    float ms2 = meshSize / 2.0;
    std::array<std::array<float, meshSize>, meshSize> meshNext;
    for (int i = 1; i < meshSize - 1; ++i)
    {
        for (int j = 1; j < meshSize - 1; ++j)
        {
            const double dtodx2 = 0.1;

            meshNext[i][j] = 2 * mesh[i][j] - meshPrior[i][j] + dtodx2 * (mesh[i+1][j] + mesh[i-1][j] + mesh[i][j-1] + mesh[i][j+1] - 4.0 * mesh[i][j]);

        }
    }
    static constexpr float diffuse = 0.05;
    for (int i = 1; i < meshSize - 1; ++i)
    {
        for (int j = 1; j < meshSize - 1; ++j)
        {
            auto meshAvg =
                0.5 * mesh[i][j] + 0.5 *
                                       (mesh[i + 1][j] + mesh[i - 1][j] + mesh[i][j - 1] +
                                        mesh[i][j + 1] + mesh[i - 1][j - 1] + mesh[i - 1][j + 1] +
                                        mesh[i + 1][j - 1] + mesh[i + 1][j + 1]) /
                                       8.0;
            meshPrior[i][j] = mesh[i][j];
            mesh[i][j] = (1-diffuse) * meshNext[i][j] + diffuse * meshAvg;

            float sz = 3 + 3 * mesh[i][j];
            float sz2 = sz / 2;
            float px = (i - ms2) / ms2;
            float py = 1.f * (meshSize - j) / meshSize;

            const float sf = 1.2;
            float xScale = (sf + py) / (sf + 1.0);
            float llx = xScale * px * cx + cx;
            float lly = py * cy + cy;
            py -= 0.5 * mesh[i][j];
            float lly2 = py * cy + cy;
            lx[i][j] = llx;
            ly[i][j] = lly;
            ly2[i][j] = lly2;
        }
    }

    auto tk = padFile.getTrack(0);
    auto it = tk->getNextIndexAtTime(priorTime);
    while (it < tk->getNumEvents() & tk->getEventTime(it) <= cp)
    {
        auto m = tk->getEventPointer(it);
        if (m->message.isNoteOn())
        {
            int margin = 3;
            auto mx = rand() % (meshSize - margin * 4) + margin;
            auto my = rand() % (meshSize - margin * 4) + margin;

            float di = rand() / RAND_MAX * 0.4 - 0.2;
            float dj = rand() / RAND_MAX * 0.4 - 0.2;
            for (int i = -margin + 1; i < margin; ++i)
            {
                for (int j = -margin + 1; j < margin; ++j)
                {
                    mesh[mx + i][my + j] = 1;
                    meshPrior[mx + i][my + j] += 1 + di * i + dj * j;
                }
            }
        }
        it++;
    }

    priorTime = cp;
    juce::Graphics g(offscreen);
    paintForReal(g);

    repaint();
}
bool SpringComponent::keyPressed(const KeyPress &key, Component *originatingComponent)
{
    if (!transportSource.isPlaying())
        transportSource.start();

    return true;
}

void SpringComponent::mouseUp(const MouseEvent &event)
{
    if (!transportSource.isPlaying())
        transportSource.start();
}
