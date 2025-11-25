#pragma once

#include <VisualizerWindowHeaders.h>

namespace TriggeredAverage
{
class MultiChannelAverageBuffer;
enum class DisplayMode : std::uint8_t;
class GridDisplay;
class TriggerSource;

class SinglePlotPanel : public Component, public ComboBox::Listener
{
public:
    SinglePlotPanel (const GridDisplay*,
                     const ContinuousChannel*,
                     const TriggerSource*,
                     int channelIndexInAverageBuffer,
                     const MultiChannelAverageBuffer*);

    void paint (Graphics& g) override;
    void resized() override;

    void clear();
    void setWindowSizeMs (float pre_ms, float post_ms);
    void setPlotType (TriggeredAverage::DisplayMode plotType);
    void setSourceColour (Colour colour);

    void setSourceName (const String& name) const;
    void drawBackground (bool);
    void setOverlayMode (bool);
    void setOverlayIndex (int index);
    void mouseMove (const MouseEvent& event) override;
    void mouseExit (const MouseEvent& event) override;
    void comboBoxChanged (ComboBox* comboBox) override;
    void update();
    void invalidateCache();

    /** Sets custom y-axis limits for the plot */
    void setYLimits (float minY, float maxY);
    
    /** Returns the current minimum y-axis limit */
    float getYMin() const { return yMin; }
    
    /** Returns the current maximum y-axis limit */
    float getYMax() const { return yMax; }
    
    /** Resets to auto-scaling mode (based on data min/max) */
    void resetYLimits();
    
    /** Returns true if using custom y-axis limits, false if auto-scaling */
    bool hasCustomYLimits() const { return useCustomYLimits; }
    
    /** Sets custom x-axis limits for the plot */
    void setXLimits (float minX, float maxX);
    
    /** Returns the current minimum x-axis limit */
    float getXMin() const { return xMin; }
    
    /** Returns the current maximum x-axis limit */
    float getXMax() const { return xMax; }
    
    /** Resets to auto X-axis scaling mode */
    void resetXLimits();
    
    /** Returns true if using custom x-axis limits, false if auto-scaling */
    bool hasCustomXLimits() const { return useCustomXLimits; }

    uint16 streamId;
    const ContinuousChannel* contChannel;
    DynamicObject getInfo() const;

private:
    struct DataRange
    {
        float minVal;
        float maxVal;
        float range;
    };

    struct TimeRange
    {
        float totalTimeMs;
        float timePerSample;
        float displayXMin;
        float displayXMax;
        float displayXRange;
    };

    DataRange calculateDataRange (const float* channelData, int numSamples);
    TimeRange calculateTimeRange (int numSamples) const;
    void plotWithDirectMapping (const float* channelData, int numSamples, const DataRange& dataRange);
    void plotWithCustomXLimits (const float* channelData, int numSamples, const DataRange& dataRange, const TimeRange& timeRange);
    void drawZeroLine (Graphics& g) const;
    bool updateCachedPath();

    std::unique_ptr<Label> infoLabel;
    std::unique_ptr<Label> channelLabel;
    std::unique_ptr<Label> conditionLabel;
    std::unique_ptr<Label> hoverLabel;
    std::unique_ptr<Label> trialCounter;

    bool plotAllTraces = true;
    bool plotAverage = true;
    int maxSortedId = 0;

    Colour baseColour;

    const TriggerSource* m_triggerSource;
    const GridDisplay* m_parentGrid;
    const MultiChannelAverageBuffer* m_averageBuffer;

    float pre_ms;
    float post_ms;
    int bin_size_ms;
    int panelWidthPx;
    int panelHeightPx;
    bool shouldDrawBackground = true;
    int overlayIndex = 0;
    bool overlayMode = false;
    bool waitingForWindowToClose;
    const double m_sampleRate;
    int channelIndexInAverageBuffer;

    // Cache for downsampled path
    Path cachedAveragePath;
    int cachedNumTrials = -1;
    int cachedPanelWidth = -1;
    int numTrials = 0;
    
    // Y-axis limits
    bool useCustomYLimits = false;
    float yMin = 0.0f;
    float yMax = 1.0f;
    
    // X-axis limits
    bool useCustomXLimits = false;
    float xMin = 0.0f;
    float xMax = 1.0f;
};
} // namespace TriggeredAverage
