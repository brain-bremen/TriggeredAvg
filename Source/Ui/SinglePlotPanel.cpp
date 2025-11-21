#include "SinglePlotPanel.h"

#include "DataCollector.h"
#include "PerformanceTimer.h"
#include "TriggerSource.h"
#include "TriggeredAvgCanvas.h"

using namespace TriggeredAverage;
const static Colour panelBackground { 30, 30, 40 };

SinglePlotPanel::SinglePlotPanel (const GridDisplay* display_,
                                  const ContinuousChannel* channel,
                                  const TriggerSource* source_,
                                  int channelIndexInAverageBuffer_,
                                  const MultiChannelAverageBuffer* avgBuffer)
    : streamId (channel->getStreamId()),
      contChannel (channel),
      baseColour (source_->colour),
      m_triggerSource (source_),
      m_parentGrid (display_),
      m_averageBuffer (avgBuffer),
      waitingForWindowToClose (false),
      m_sampleRate (channel->getSampleRate()),
      channelIndexInAverageBuffer (channelIndexInAverageBuffer_)
{
    pre_ms = 0;
    post_ms = 0;
    bin_size_ms = 10;

    infoLabel = std::make_unique<Label> ("info label");
    infoLabel->setJustificationType (Justification::topLeft);
    infoLabel->setText (channel->getName(), dontSendNotification);
    infoLabel->setColour (Label::textColourId, Colours::white);
    addAndMakeVisible (infoLabel.get());

    const auto font12pt = Font { withDefaultMetrics (FontOptions { 12.0f }) };
    const auto font16pt = Font { withDefaultMetrics (FontOptions { 16.0f }) };

    channelLabel = std::make_unique<Label> ("channel label");
    channelLabel->setFont (font12pt);
    channelLabel->setJustificationType (Justification::topLeft);
    channelLabel->setColour (Label::textColourId, Colours::white);
    String channelString = "";
    for (auto ch : channel->getName())
        channelString += ch + ", ";

    channelString = channelString.substring (0, channelString.length() - 2);
    channelLabel->setText (channelString, dontSendNotification);
    addAndMakeVisible (channelLabel.get());

    conditionLabel = std::make_unique<Label> ("condition label");
    conditionLabel->setFont (font16pt);
    conditionLabel->setJustificationType (Justification::topLeft);
    conditionLabel->setText (m_triggerSource->name, dontSendNotification);
    conditionLabel->setColour (Label::textColourId, baseColour);
    addAndMakeVisible (conditionLabel.get());

    hoverLabel = std::make_unique<Label> ("hover label");
    hoverLabel->setJustificationType (Justification::topLeft);
    hoverLabel->setFont (font12pt);
    hoverLabel->setColour (Label::textColourId, Colours::white);
    addAndMakeVisible (hoverLabel.get());

    trialCounter = std::make_unique<Label> ("trial counter");
    trialCounter->setFont (font12pt);
    trialCounter->setJustificationType (Justification::centredTop);
    juce::String trialCounterString = String ("Trials: ") + String (numTrials);
    trialCounter->setText (trialCounterString, dontSendNotification);
    trialCounter->setColour (Label::textColourId, baseColour);
    addAndMakeVisible (trialCounter.get());

    clear();
}

void SinglePlotPanel::resized()
{
    int labelOffset;
    const int width = getWidth();

    if (width < 320)
        labelOffset = 5;
    else
        labelOffset = width - 150;

    if (labelOffset == 5)
        panelWidthPx = width - labelOffset;
    else
        panelWidthPx = labelOffset - 10;

    panelHeightPx = (getHeight() - 10);

    // Invalidate cache when size changes
    if (cachedPanelWidth != panelWidthPx)
    {
        pathNeedsUpdate = true;
        cachedPanelWidth = panelWidthPx;
    }

    infoLabel->setBounds (labelOffset, 10, 150, 30);

    if (getHeight() < 100)
    {
        conditionLabel->setBounds (labelOffset, 26, 150, 30);
        channelLabel->setVisible (false);
        hoverLabel->setVisible (false);
    }
    else
    {
        conditionLabel->setBounds (labelOffset, 49, 150, 15);
        channelLabel->setVisible (! overlayMode);
        channelLabel->setBounds (labelOffset, 26, 150, 30);

        hoverLabel->setVisible (! overlayMode);
        hoverLabel->setBounds (labelOffset, 66, 150, 45);
    }

    if (labelOffset == 5)
    {
        conditionLabel->setVisible (false);
        channelLabel->setVisible (false);
        hoverLabel->setBounds (width - 120, 10, 150, 45);
    }
    else
    {
        conditionLabel->setVisible (true);
        channelLabel->setVisible (! overlayMode);

        if (overlayMode)
        {
            conditionLabel->setBounds (labelOffset, 49 + 18 * overlayIndex, 150, 15);
        }
    }

    trialCounter->setBounds (labelOffset, 85, 150, 20);  // Add this line
}

void SinglePlotPanel::clear()
{
    numTrials = 0;
    pathNeedsUpdate = true;
    cachedNumTrials = -1;
}

void SinglePlotPanel::setYLimits (float minY, float maxY)
{
    if (minY >= maxY)
    {
        // Invalid range, ignore
        return;
    }
    
    yMin = minY;
    yMax = maxY;
    useCustomYLimits = true;
    pathNeedsUpdate = true;
    repaint();
}

void SinglePlotPanel::resetYLimits()
{
    useCustomYLimits = false;
    pathNeedsUpdate = true;
    repaint();
}

void SinglePlotPanel::setWindowSizeMs (float pre, float post)
{
    pre_ms = pre;
    post_ms = post;
}

void SinglePlotPanel::setPlotType (TriggeredAverage::DisplayMode plotType)
{
    switch (plotType)
    {
        case DisplayMode::INDIVIDUAL_TRACES:
            plotAverage = false;
            plotAllTraces = true;
            break;
        case DisplayMode::AVERAGE_TRAGE:
            plotAverage = true;
            plotAllTraces = false;
            break;
        case DisplayMode::ALL_AND_AVERAGE:
            plotAverage = true;
            plotAllTraces = true;
            break;
        default:
            plotAverage = true;
            plotAllTraces = false;
            break;
    }

    repaint();
}

void SinglePlotPanel::setSourceColour (Colour colour)
{
    baseColour = colour;
    conditionLabel->setColour (Label::textColourId, baseColour);
    repaint();
}

void SinglePlotPanel::setSourceName (const String& name) const
{
    conditionLabel->setText (name, dontSendNotification);
}

void SinglePlotPanel::drawBackground (bool shouldDraw)
{
    shouldDrawBackground = shouldDraw;

    infoLabel->setVisible (shouldDrawBackground);
}

void SinglePlotPanel::setOverlayMode (bool shouldOverlay) { overlayMode = shouldOverlay; }

void SinglePlotPanel::setOverlayIndex (int index)
{
    overlayIndex = index;
    resized();
}

void SinglePlotPanel::update()
{
    numTrials++;
    pathNeedsUpdate = true;
}

void SinglePlotPanel::invalidateCache()
{
    pathNeedsUpdate = true;
}

void SinglePlotPanel::paint (Graphics& g)
{
    PerformanceTimer totalTimer ("SinglePlotPanel::paint", 10.0); // Only log if > 10ms

    if (shouldDrawBackground)
    {
        g.fillAll (panelBackground);
    }

    if (plotAverage && m_averageBuffer)
    {
        // Check if we need to update the cached path
        int currentNumTrials = m_averageBuffer->getNumTrials();
        bool needsUpdate =
            pathNeedsUpdate || (currentNumTrials != cachedNumTrials) || cachedAveragePath.isEmpty();

        AudioBuffer<float> avgBuffer;
        if (needsUpdate)
        {
            PerformanceTimer updateTimer ("update cached path", 5.0);

            {
                PerformanceTimer avgTimer ("getAverage()", 5.0); // Only log if > 5ms
                avgBuffer = m_averageBuffer->getAverage();
            }

            auto nSamples = avgBuffer.getNumSamples();
            auto nChannels = avgBuffer.getNumChannels();
            auto trialCounterString = String (m_averageBuffer->getNumTrials());
            trialCounter->setText (trialCounterString, dontSendNotification);

            if (avgBuffer.getNumSamples() > 0 && avgBuffer.getNumChannels() > 0)
            {
                const int numSamples = avgBuffer.getNumSamples();
                const float* channelData = avgBuffer.getReadPointer (channelIndexInAverageBuffer);

                // Calculate min/max for scaling
                float minVal, maxVal;
                
                if (useCustomYLimits)
                {
                    // Use custom y-axis limits
                    minVal = yMin;
                    maxVal = yMax;
                }
                else
                {
                    // Auto-scale based on data
                    minVal = channelData[0];
                    maxVal = channelData[0];
                    for (int i = 1; i < numSamples; ++i)
                    {
                        minVal = std::min (minVal, channelData[i]);
                        maxVal = std::max (maxVal, channelData[i]);
                    }
                }

                float range = maxVal - minVal;
                if (range < 1e-6f)
                    range = 1.0f;

                // Downsample to pixel resolution
                const int numPixels = panelWidthPx;
                const int samplesPerPixel = std::max (1, numSamples / numPixels);

                cachedAveragePath.clear();

                if (samplesPerPixel <= 1)
                {
                    // No downsampling needed - fewer samples than pixels
                    for (int i = 0; i < numSamples; ++i)
                    {
                        float x = (static_cast<float> (i) / static_cast<float> (numSamples - 1))
                                  * static_cast<float> (panelWidthPx);
                        
                        // Clamp value to range if using custom limits
                        float value = channelData[i];
                        if (useCustomYLimits)
                        {
                            value = std::max (minVal, std::min (maxVal, value));
                        }
                        
                        float normalizedValue = (value - minVal) / range;
                        float y = static_cast<float> (panelHeightPx) * (1.0f - normalizedValue);

                        if (i == 0)
                            cachedAveragePath.startNewSubPath (x, y);
                        else
                            cachedAveragePath.lineTo (x, y);
                    }
                }
                else
                {
                    // Downsample using min-max envelope to preserve peaks and troughs
                    for (int pixelIndex = 0; pixelIndex < numPixels; ++pixelIndex)
                    {
                        int sampleStart = pixelIndex * samplesPerPixel;
                        int sampleEnd = std::min (sampleStart + samplesPerPixel, numSamples);

                        // Find min and max within this pixel's sample range
                        float pixelMin = channelData[sampleStart];
                        float pixelMax = channelData[sampleStart];

                        for (int i = sampleStart + 1; i < sampleEnd; ++i)
                        {
                            pixelMin = std::min (pixelMin, channelData[i]);
                            pixelMax = std::max (pixelMax, channelData[i]);
                        }
                        
                        // Clamp to range if using custom limits
                        if (useCustomYLimits)
                        {
                            pixelMin = std::max (minVal, std::min (maxVal, pixelMin));
                            pixelMax = std::max (minVal, std::min (maxVal, pixelMax));
                        }

                        float x = static_cast<float> (pixelIndex);

                        // Draw vertical line from min to max for this pixel
                        float yMin = static_cast<float> (panelHeightPx)
                                     * (1.0f - (pixelMin - minVal) / range);
                        float yMax = static_cast<float> (panelHeightPx)
                                     * (1.0f - (pixelMax - minVal) / range);

                        if (pixelIndex == 0)
                        {
                            cachedAveragePath.startNewSubPath (x, yMin);
                        }
                        else
                        {
                            cachedAveragePath.lineTo (x, yMin);
                        }

                        // Draw to max if there's a range in this pixel
                        if (std::abs (yMax - yMin) > 0.5f)
                        {
                            cachedAveragePath.lineTo (x, yMax);
                        }
                    }
                }

                cachedNumTrials = currentNumTrials;
                pathNeedsUpdate = false;
            }
        }

        // Draw the cached path (fast!)
        if (! cachedAveragePath.isEmpty())
        {
            g.setColour (baseColour);
            g.strokePath (cachedAveragePath, PathStrokeType (1.0f));
        }
    }

    g.setColour (Colours::white);

    // t = 0
    float zeroLoc = (pre_ms) / (pre_ms + post_ms) * static_cast<float> (panelWidthPx);
    g.drawLine (zeroLoc, 0, zeroLoc, static_cast<float> (getHeight()), 2.0);
}

void SinglePlotPanel::mouseMove (const MouseEvent& event)
{
    if (event.getPosition().x < panelWidthPx)
    {
        String hoverString = "TODO";
        hoverLabel->setText (hoverString, dontSendNotification);

        repaint();
    }
}

void SinglePlotPanel::mouseExit (const MouseEvent& event)
{
    hoverLabel->setText ("", dontSendNotification);
    repaint();
}

void SinglePlotPanel::comboBoxChanged (ComboBox* comboBox)
{
    if (overlayMode)
    {
        //display->setUnitForElectrode (spikeChannel,
        //                              uniqueSortedIds[comboBox->getSelectedItemIndex()]);
    }
    else
    {
        repaint();
    }
}

DynamicObject SinglePlotPanel::getInfo() const
{
    DynamicObject info;

    info.setProperty (Identifier ("channel"), var (contChannel->getName()));
    info.setProperty (Identifier ("condition"), var (m_triggerSource->name));
    info.setProperty (Identifier ("color"), var (m_triggerSource->colour.toString()));
    info.setProperty (Identifier ("trial_count"), var (int (numTrials)));

    return info;
}
