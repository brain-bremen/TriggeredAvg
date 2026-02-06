#pragma once
#include "SinglePlotPanel.h"
#include "DisplayMode.h"
#include <VisualizerWindowHeaders.h>

namespace TriggeredAverage
{
class MultiChannelAverageBuffer;
class TriggerSource;

// GUI Component that holds the grid of triggered average panels
class GridDisplay : public Component
{
public:
    GridDisplay();
    ~GridDisplay();

    /** Renders the Visualizer on each animation callback cycle
        Called instead of Juce's "repaint()" to avoid redrawing underlying components
        if not necessary.*/
    void refresh();

    void resized() override;
    void setWindowSizeMs (float pre_ms, float post_ms);
    void setPlotType (TriggeredAverage::DisplayMode plotType);

    void addContChannel (const ContinuousChannel*,
                         const TriggerSource*,
                         int channelIndexInAverageBuffer,
                         const MultiChannelAverageBuffer*);

    void updateColourForSource (const TriggerSource* source);
    void updateConditionName (const TriggerSource* source);
    void setNumColumns (int numColumns);
    void setRowHeight (int rowHeightPixels);

    void setConditionOverlay (bool);
    void prepareToUpdate();
    int getDesiredHeight() const;
    void clearPanels();

    /** Sets custom y-axis limits for all panels */
    void setYLimits (float minY, float maxY);

    /** Resets all panels to auto-scaling mode */
    void resetYLimits();

    /** Sets custom x-axis limits for all panels */
    void setXLimits (float minX, float maxX);

    /** Resets all panels to auto X-axis scaling mode */
    void resetXLimits();

    /** Sets custom y-axis limits for panels associated with a specific trigger source */
    void setYLimitsForSource (const TriggerSource* source, float minY, float maxY);

    /** Sets custom y-axis limits for panels associated with a specific channel */
    void setYLimitsForChannel (const ContinuousChannel* channel, float minY, float maxY);

    DynamicObject getInfo();

    /** Configures individual trial display settings for all panels */
    void setShowIndividualTrials (bool show);

    /** Sets the maximum number of trials to display for all panels */
    void setMaxTrialsToDisplay (int n);

    /** Sets the opacity for individual trial traces for all panels */
    void setTrialOpacity (float opacity);

    /** Connects trial buffers to panels for a given trigger source */
    void setTrialBuffersForSource (const TriggerSource* source,
                                   const class SingleTrialBuffer* trialBuffer);

private:
    OwnedArray<SinglePlotPanel> panels;

    std::unordered_map<const TriggerSource*, Array<SinglePlotPanel*>> triggerSourceToPanelMap;
    std::unordered_map<const ContinuousChannel*, Array<SinglePlotPanel*>> contChannelToPanelMap;

    int totalHeight = 0;
    int panelHeightPx = 150;
    int borderSize = 10;
    int numColumns = 1;

    bool overlayConditions = false;

    float post_ms;
    DisplayMode plotType = DisplayMode::INDIVIDUAL_TRACES;
};

} // namespace TriggeredAverage
