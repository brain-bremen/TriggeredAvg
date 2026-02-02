#include "TriggeredAvgCanvas.h"
#include "GridDisplay.h"
#include "TimeAxis.h"
#include "TriggeredAvgNode.h"

using namespace TriggeredAverage;

OptionsBar::OptionsBar (TriggeredAvgCanvas* canvas_, GridDisplay* display_, TimeAxis* timescale_)
    : display (display_),
      canvas (canvas_),
      timescale (timescale_)
{
    clearButton = std::make_unique<UtilityButton> ("CLEAR");

    clearButton->setFont (FontOptions (12.0f));
    clearButton->addListener (this);
    clearButton->setClickingTogglesState (false);
    addAndMakeVisible (clearButton.get());

    saveButton = std::make_unique<UtilityButton> ("SAVE");
    saveButton->setFont (FontOptions (12.0f));
    saveButton->addListener (this);
    saveButton->setClickingTogglesState (false);
    addAndMakeVisible (saveButton.get());

    plotTypeSelector = std::make_unique<ComboBox> ("Plot Type Selector");

    plotTypeSelector->addItemList (DisplayModeStrings, 1);

    plotTypeSelector->setSelectedId (1, dontSendNotification);
    plotTypeSelector->addListener (this);
    addAndMakeVisible (plotTypeSelector.get());

    columnNumberSelector = std::make_unique<ComboBox> ("Column Number Selector");
    for (int i = 1; i < 7; i++)
        columnNumberSelector->addItem (String (i), i);
    columnNumberSelector->setSelectedId (1, dontSendNotification);
    columnNumberSelector->addListener (this);
    addAndMakeVisible (columnNumberSelector.get());

    rowHeightSelector = std::make_unique<ComboBox> ("Row Height Selector");
    for (int i = 2; i < 6; i++)
        rowHeightSelector->addItem (String (i * 50) + " px", i * 50);
    rowHeightSelector->setSelectedId (150, dontSendNotification);
    rowHeightSelector->addListener (this);
    addAndMakeVisible (rowHeightSelector.get());

    overlayButton = std::make_unique<UtilityButton> ("OFF");
    overlayButton->setFont (FontOptions (12.0f));
    overlayButton->addListener (this);
    overlayButton->setClickingTogglesState (true);
    addAndMakeVisible (overlayButton.get());

    // X-axis limit controls
    xLimitsToggle = std::make_unique<UtilityButton> ("AUTO");
    xLimitsToggle->setFont (FontOptions (12.0f));
    xLimitsToggle->addListener (this);
    xLimitsToggle->setClickingTogglesState (true);
    addAndMakeVisible (xLimitsToggle.get());

    xMinLabel = std::make_unique<Label> ("X Min Label", "Min:");
    xMinLabel->setFont (FontOptions (12.0f));
    xMinLabel->setJustificationType (Justification::centredRight);
    addAndMakeVisible (xMinLabel.get());

    xMaxLabel = std::make_unique<Label> ("X Max Label", "Max:");
    xMaxLabel->setFont (FontOptions (12.0f));
    xMaxLabel->setJustificationType (Justification::centredRight);
    addAndMakeVisible (xMaxLabel.get());

    xMinEditor = std::make_unique<TextEditor> ("X Min");
    xMinEditor->setText ("-50.0");
    xMinEditor->setFont (FontOptions (12.0f));
    xMinEditor->setEnabled (false);
    xMinEditor->onReturnKey = [this]() { updateXLimits(); };
    xMinEditor->onFocusLost = [this]() { updateXLimits(); };
    addAndMakeVisible (xMinEditor.get());

    xMaxEditor = std::make_unique<TextEditor> ("X Max");
    xMaxEditor->setText ("50.0");
    xMaxEditor->setFont (FontOptions (12.0f));
    xMaxEditor->setEnabled (false);
    xMaxEditor->onReturnKey = [this]() { updateXLimits(); };
    xMaxEditor->onFocusLost = [this]() { updateXLimits(); };
    addAndMakeVisible (xMaxEditor.get());


    // Y-axis limit controls
    yLimitsToggle = std::make_unique<UtilityButton> ("AUTO");
    yLimitsToggle->setFont (FontOptions (12.0f));
    yLimitsToggle->addListener (this);
    yLimitsToggle->setClickingTogglesState (true);
    addAndMakeVisible (yLimitsToggle.get());

    yMinLabel = std::make_unique<Label> ("Y Min Label", "Min:");
    yMinLabel->setFont (FontOptions (12.0f));
    yMinLabel->setJustificationType (Justification::centredRight);
    addAndMakeVisible (yMinLabel.get());

    yMaxLabel = std::make_unique<Label> ("Y Max Label", "Max:");
    yMaxLabel->setFont (FontOptions (12.0f));
    yMaxLabel->setJustificationType (Justification::centredRight);
    addAndMakeVisible (yMaxLabel.get());

    yMinEditor = std::make_unique<TextEditor> ("Y Min");
    yMinEditor->setText ("-100.0");
    yMinEditor->setFont (FontOptions (12.0f));
    yMinEditor->setEnabled (false);
    yMinEditor->onReturnKey = [this]() { updateYLimits(); };
    yMinEditor->onFocusLost = [this]() { updateYLimits(); };
    addAndMakeVisible (yMinEditor.get());

    yMaxEditor = std::make_unique<TextEditor> ("Y Max");
    yMaxEditor->setText ("100.0");
    yMaxEditor->setFont (FontOptions (12.0f));
    yMaxEditor->setEnabled (false);
    yMaxEditor->onReturnKey = [this]() { updateYLimits(); };
    yMaxEditor->onFocusLost = [this]() { updateYLimits(); };
    addAndMakeVisible (yMaxEditor.get());

    // Individual trial display controls
    showTrialsToggle = std::make_unique<UtilityButton> ("OFF");
    showTrialsToggle->setFont (FontOptions (12.0f));
    showTrialsToggle->addListener (this);
    showTrialsToggle->setClickingTogglesState (true);
    addAndMakeVisible (showTrialsToggle.get());

    numTrialsLabel = std::make_unique<Label> ("Num Trials Label", "N:");
    numTrialsLabel->setFont (FontOptions (12.0f));
    numTrialsLabel->setJustificationType (Justification::centredRight);
    addAndMakeVisible (numTrialsLabel.get());

    numTrialsSelector = std::make_unique<ComboBox> ("Number of Trials");
    for (int i = 1; i <= 50; i += (i < 10 ? 1 : (i < 20 ? 5 : 10)))
        numTrialsSelector->addItem (String (i), i);
    numTrialsSelector->setSelectedId (10, dontSendNotification);
    numTrialsSelector->addListener (this);
    numTrialsSelector->setEnabled (false);  // Initially disabled
    addAndMakeVisible (numTrialsSelector.get());

    trialOpacityLabel = std::make_unique<Label> ("Opacity Label", "?:");
    trialOpacityLabel->setFont (FontOptions (12.0f));
    trialOpacityLabel->setJustificationType (Justification::centredRight);
    addAndMakeVisible (trialOpacityLabel.get());

    trialOpacitySlider = std::make_unique<Slider> (Slider::LinearHorizontal, Slider::NoTextBox);
    trialOpacitySlider->setRange (0.1, 1.0, 0.05);
    trialOpacitySlider->setValue (0.3);
    trialOpacitySlider->onValueChange = [this]() { updateTrialDisplaySettings(); };
    trialOpacitySlider->setEnabled (false);  // Initially disabled
    addAndMakeVisible (trialOpacitySlider.get());
}

void OptionsBar::buttonClicked (Button* button)
{
    if (button == clearButton.get())
    {
        display->clearPanels();
    }

    else if (button == overlayButton.get())
    {
        display->setConditionOverlay (button->getToggleState());

        if (overlayButton->getToggleState())
            overlayButton->setLabel ("ON");
        else
            overlayButton->setLabel ("OFF");

        canvas->resized();
    }
    else if (button == yLimitsToggle.get())
    {
        useCustomYLimits = button->getToggleState();

        if (useCustomYLimits)
        {
            yLimitsToggle->setLabel ("MANUAL");
            yMinEditor->setEnabled (true);
            yMaxEditor->setEnabled (true);
            updateYLimits();
        }
        else
        {
            yLimitsToggle->setLabel ("AUTO");
            yMinEditor->setEnabled (false);
            yMaxEditor->setEnabled (false);
            display->resetYLimits();
        }

        // Notify the processor of the change
        if (auto* processor = canvas->getProcessor())
        {
            if (auto* triggeredAvgNode = dynamic_cast<TriggeredAvgNode*> (processor))
            {
                triggeredAvgNode->getParameter (ParameterNames::use_custom_y_limits)->setNextValue (useCustomYLimits ? 1.0f : 0.0f, false);
            }
        }
    }
    else if (button == xLimitsToggle.get())
    {
        useCustomXLimits = button->getToggleState();

        if (useCustomXLimits)
        {
            xLimitsToggle->setLabel ("MANUAL");
            xMinEditor->setEnabled (true);
            xMaxEditor->setEnabled (true);
            updateXLimits();
        }
        else
        {
            xLimitsToggle->setLabel ("AUTO");
            xMinEditor->setEnabled (false);
            xMaxEditor->setEnabled (false);
            display->resetXLimits();
        }

        // Notify the processor of the change
        if (auto* processor = canvas->getProcessor())
        {
            if (auto* triggeredAvgNode = dynamic_cast<TriggeredAvgNode*> (processor))
            {
                triggeredAvgNode->getParameter (ParameterNames::use_custom_x_limits)->setNextValue (useCustomXLimits ? 1.0f : 0.0f, false);
            }
        }
    }
    else if (button == showTrialsToggle.get())
    {
        showTrials = button->getToggleState();

        if (showTrials)
        {
            showTrialsToggle->setLabel ("ON");
            numTrialsSelector->setEnabled (true);
            trialOpacitySlider->setEnabled (true);
        }
        else
        {
            showTrialsToggle->setLabel ("OFF");
            numTrialsSelector->setEnabled (false);
            trialOpacitySlider->setEnabled (false);
        }

        updateTrialDisplaySettings();
    }
    else if (button == saveButton.get())
    {
        DynamicObject output = display->getInfo();

        FileChooser chooser ("Save histogram statistics to file...", File(), "*.json");

        if (chooser.browseForFileToSave (true))
        {
            File file = chooser.getResult();

            if (file.exists())
                file.deleteFile();

            FileOutputStream f (file);

            output.writeAsJSON (f,
                                JSON::FormatOptions {}
                                    .withIndentLevel (5)
                                    .withSpacing (JSON::Spacing::multiLine)
                                    .withMaxDecimalPlaces (4));
        }
    }
}

void OptionsBar::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == plotTypeSelector.get())
    {
        auto id = comboBox->getSelectedId();
        display->setPlotType (static_cast<DisplayMode> (comboBox->getSelectedId()));
    }
    else if (comboBox == columnNumberSelector.get())
    {
        const int numColumns = comboBox->getSelectedId();

        display->setNumColumns (numColumns);

        if (numColumns == 1)
            timescale->setVisible (true);
        else
            timescale->setVisible (false);

        canvas->resized();
    }
    else if (comboBox == rowHeightSelector.get())
    {
        display->setRowHeight (comboBox->getSelectedId());

        canvas->resized();
    }
    else if (comboBox == numTrialsSelector.get())
    {
        maxTrialsToDisplay = comboBox->getSelectedId();
        updateTrialDisplaySettings();
    }
}

void OptionsBar::resized()
{
    const int verticalOffset = 7;

    clearButton->setBounds (getWidth() - 100, verticalOffset, 70, 25);
    saveButton->setBounds (getWidth() - 180, verticalOffset, 70, 25);

    plotTypeSelector->setBounds (440, verticalOffset, 150, 25);

    rowHeightSelector->setBounds (60, verticalOffset, 80, 25);

    columnNumberSelector->setBounds (200, verticalOffset, 50, 25);

    overlayButton->setBounds (340, verticalOffset, 35, 25);

    // X-axis limit controls
    xLimitsToggle->setBounds (610, verticalOffset, 65, 25);
    xMinLabel->setBounds (685, verticalOffset, 35, 25);
    xMinEditor->setBounds (720, verticalOffset, 60, 25);
    xMaxLabel->setBounds (790, verticalOffset, 35, 25);
    xMaxEditor->setBounds (825, verticalOffset, 60, 25);

    // Y-axis limit controls - positioned to the right of X-axis controls
    yLimitsToggle->setBounds (905, verticalOffset, 65, 25);
    yMinLabel->setBounds (980, verticalOffset, 35, 25);
    yMinEditor->setBounds (1015, verticalOffset, 60, 25);
    yMaxLabel->setBounds (1085, verticalOffset, 35, 25);
    yMaxEditor->setBounds (1120, verticalOffset, 60, 25);

    // Individual trial display controls - positioned after Y-axis controls
    showTrialsToggle->setBounds (1200, verticalOffset, 35, 25);
    numTrialsLabel->setBounds (1245, verticalOffset, 20, 25);
    numTrialsSelector->setBounds (1265, verticalOffset, 55, 25);
    trialOpacityLabel->setBounds (1330, verticalOffset, 20, 25);
    trialOpacitySlider->setBounds (1350, verticalOffset, 80, 25);
}

void OptionsBar::paint (Graphics& g)
{
    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (FontOptions ("Inter", "Regular", 15.0f));

    const int verticalOffset = 4;

    g.drawText ("Row", 0, verticalOffset, 53, 15, Justification::centredRight, false);
    g.drawText ("Height", 0, verticalOffset + 15, 53, 15, Justification::centredRight, false);
    g.drawText ("Num", 150, verticalOffset, 43, 15, Justification::centredRight, false);
    g.drawText ("Cols", 150, verticalOffset + 15, 43, 15, Justification::centredRight, false);
    g.drawText ("Overlay", 240, verticalOffset, 93, 15, Justification::centredRight, false);
    g.drawText ("Conditions", 240, verticalOffset + 15, 93, 15, Justification::centredRight, false);
    g.drawText ("Plot", 390, verticalOffset, 43, 15, Justification::centredRight, false);
    g.drawText ("Type", 390, verticalOffset + 15, 43, 15, Justification::centredRight, false);
    g.drawText ("X-Axis", 600, verticalOffset, 70, 15, Justification::centred, false);
    g.drawText ("Limits", 600, verticalOffset + 15, 70, 15, Justification::centred, false);
    g.drawText ("Y-Axis", 895, verticalOffset, 70, 15, Justification::centred, false);
    g.drawText ("Limits", 895, verticalOffset + 15, 70, 15, Justification::centred, false);
    g.drawText ("Show", 1185, verticalOffset, 50, 15, Justification::centred, false);
    g.drawText ("Trials", 1185, verticalOffset + 15, 50, 15, Justification::centred, false);
}

void OptionsBar::updateYLimits()
{
    if (!useCustomYLimits)
        return;

    float minY = yMinEditor->getText().getFloatValue();
    float maxY = yMaxEditor->getText().getFloatValue();

    if (minY >= maxY)
    {
        // Invalid range - reset to defaults
        yMinEditor->setText ("-100.0");
        yMaxEditor->setText ("100.0");
        minY = -100.0f;
        maxY = 100.0f;
    }

    display->setYLimits (minY, maxY);

    // Notify the processor of the changes
    if (auto* processor = canvas->getProcessor())
    {
        if (auto* triggeredAvgNode = dynamic_cast<TriggeredAvgNode*> (processor))
        {
            triggeredAvgNode->getParameter (ParameterNames::y_min)->setNextValue (minY, false);
            triggeredAvgNode->getParameter (ParameterNames::y_max)->setNextValue (maxY, false);
        }
    }
}

void OptionsBar::updateXLimits()
{
    if (!useCustomXLimits)
        return;

    float minX = xMinEditor->getText().getFloatValue();
    float maxX = xMaxEditor->getText().getFloatValue();

    if (minX >= maxX)
    {
        // Invalid range - reset to defaults
        xMinEditor->setText ("-50.0");
        xMaxEditor->setText ("50.0");
        minX = -50.0f;
        maxX = 50.0f;
    }

    display->setXLimits (minX, maxX);

    // Notify the processor of the changes
    if (auto* processor = canvas->getProcessor())
    {
        if (auto* triggeredAvgNode = dynamic_cast<TriggeredAvgNode*> (processor))
        {
            triggeredAvgNode->getParameter (ParameterNames::x_min)->setNextValue (minX, false);
            triggeredAvgNode->getParameter (ParameterNames::x_max)->setNextValue (maxX, false);
        }
    }
}

void OptionsBar::updateTrialDisplaySettings()
{
    // Update local state
    trialOpacity = (float) trialOpacitySlider->getValue();

    // Propagate settings to all panels via GridDisplay
    display->setMaxTrialsToDisplay (maxTrialsToDisplay);
    display->setTrialOpacity (trialOpacity);

    // Note: Trial buffers need to be connected when panels are created
    // This is handled in TriggeredAvgNode when it calls addContChannel
}

void OptionsBar::saveCustomParametersToXml (XmlElement* xml) const
{
    xml->setAttribute ("plot_type", plotTypeSelector->getSelectedId());
    xml->setAttribute ("num_cols", columnNumberSelector->getSelectedId());
    xml->setAttribute ("row_height", rowHeightSelector->getSelectedId());
    xml->setAttribute ("overlay", overlayButton->getToggleState());

    // Save X-axis limit parameters
    xml->setAttribute ("use_custom_x_limits", useCustomXLimits);
    if (useCustomXLimits)
    {
        xml->setAttribute ("x_min", xMinEditor->getText().getFloatValue());
        xml->setAttribute ("x_max", xMaxEditor->getText().getFloatValue());
    }

    // Save Y-axis limit parameters
    xml->setAttribute ("use_custom_y_limits", useCustomYLimits);
    if (useCustomYLimits)
    {
        xml->setAttribute ("y_min", yMinEditor->getText().getFloatValue());
        xml->setAttribute ("y_max", yMaxEditor->getText().getFloatValue());
    }

    // Save individual trial display parameters
    xml->setAttribute ("show_trials", showTrials);
    xml->setAttribute ("max_trials_to_display", maxTrialsToDisplay);
    xml->setAttribute ("trial_opacity", trialOpacity);
}

void OptionsBar::loadCustomParametersFromXml (XmlElement* xml)
{
    columnNumberSelector->setSelectedId (xml->getIntAttribute ("num_cols", 1), sendNotification);
    rowHeightSelector->setSelectedId (xml->getIntAttribute ("row_height", 150), sendNotification);
    overlayButton->setToggleState (xml->getBoolAttribute ("overlay", false), sendNotification);
    plotTypeSelector->setSelectedId (xml->getIntAttribute ("plot_type", 1), sendNotification);

    // Load X-axis limit parameters
    bool customXLimits = xml->getBoolAttribute ("use_custom_x_limits", false);

    if (customXLimits)
    {
        float minX = (float) xml->getDoubleAttribute ("x_min", -50.0);
        float maxX = (float) xml->getDoubleAttribute ("x_max", 50.0);

        xMinEditor->setText (String (minX));
        xMaxEditor->setText (String (maxX));

        xLimitsToggle->setToggleState (true, sendNotification);
    }
    else
    {
        xLimitsToggle->setToggleState (false, sendNotification);
    }

    // Load Y-axis limit parameters
    bool customYLimits = xml->getBoolAttribute ("use_custom_y_limits", false);

    if (customYLimits)
    {
        float minY = (float) xml->getDoubleAttribute ("y_min", -100.0);
        float maxY = (float) xml->getDoubleAttribute ("y_max", 100.0);

        yMinEditor->setText (String (minY));
        yMaxEditor->setText (String (maxY));

        yLimitsToggle->setToggleState (true, sendNotification);
    }
    else
    {
        yLimitsToggle->setToggleState (false, sendNotification);
    }

    // Load individual trial display parameters
    showTrials = xml->getBoolAttribute ("show_trials", false);
    maxTrialsToDisplay = xml->getIntAttribute ("max_trials_to_display", 10);
    trialOpacity = (float) xml->getDoubleAttribute ("trial_opacity", 0.3);

    // Update UI controls
    showTrialsToggle->setToggleState (showTrials, sendNotification);
    numTrialsSelector->setSelectedId (maxTrialsToDisplay, sendNotification);
    trialOpacitySlider->setValue (trialOpacity, sendNotification);
}

TriggeredAvgCanvas::TriggeredAvgCanvas (TriggeredAvgNode* processor_)
    : Visualizer (processor_),
      m_dataStore (processor_->getDataStore())
{
    m_timeAxis = std::make_unique<TimeAxis>();
    addAndMakeVisible (m_timeAxis.get());

    m_mainViewport = std::make_unique<Viewport>();
    m_mainViewport->setScrollBarsShown (true, true);

    m_grid = std::make_unique<GridDisplay>();
    m_mainViewport->setViewedComponent (m_grid.get(), false);
    m_mainViewport->setScrollBarThickness (15);
    addAndMakeVisible (m_mainViewport.get());
    m_grid->setBounds (0, 50, 500, 100);

    m_optionsBarHolder = std::make_unique<Viewport>();
    m_optionsBarHolder->setScrollBarsShown (false, true);
    m_optionsBarHolder->setScrollBarThickness (10);

    m_optionsBar = std::make_unique<OptionsBar> (this, m_grid.get(), m_timeAxis.get());
    m_optionsBarHolder->setViewedComponent (m_optionsBar.get(), false);
    addAndMakeVisible (m_optionsBarHolder.get());

    // Start timer for regular display updates (60 Hz)
    // Note: Visualizer already inherits from Timer, so we use the inherited startTimer
    startTimer (16);  // ~60 FPS
}

void TriggeredAvgCanvas::refreshState() { resized(); }

void TriggeredAvgCanvas::resized()
{
    const int scrollBarThickness = m_mainViewport->getScrollBarThickness();
    const int timescaleHeight = 40;
    const int optionsBarHeight = 44;

    if (m_timeAxis->isVisible())
    {
        m_timeAxis->setBounds (10, 0, getWidth() - scrollBarThickness - 150, timescaleHeight);
        m_mainViewport->setBounds (
            0, timescaleHeight, getWidth(), getHeight() - timescaleHeight - optionsBarHeight);
    }
    else
    {
        m_mainViewport->setBounds (0, 10, getWidth(), getHeight() - 10 - optionsBarHeight);
    }

    m_grid->setBounds (0, 0, getWidth() - scrollBarThickness, m_grid->getDesiredHeight());
    m_grid->resized();

    m_optionsBarHolder->setBounds (0, getHeight() - optionsBarHeight, getWidth(), optionsBarHeight);

    int optionsWidth = getWidth() < 775 ? 775 : getWidth();
    m_optionsBar->setBounds (0, 0, optionsWidth, m_optionsBarHolder->getHeight());
}

void TriggeredAvgCanvas::paint (Graphics& g)
{
    g.fillAll (Colour (0, 18, 43));

    g.setColour (findColour (ThemeColours::componentBackground));
    g.fillRect (m_optionsBarHolder->getBounds());
}

void TriggeredAvgCanvas::setWindowSizeMs (float pre_ms_, float post_ms_)
{
    pre_ms = pre_ms_;
    post_ms = post_ms_;

    m_grid->setWindowSizeMs (pre_ms, post_ms);
    m_timeAxis->setWindowSizeMs (pre_ms, post_ms);

    repaint();
}

void TriggeredAvgCanvas::addContChannel (const ContinuousChannel* channel,
                                         const TriggerSource* source,
                                         int channelIndexInAverageBuffer,
                                         const MultiChannelAverageBuffer* avgBuffer)
{
    m_grid->addContChannel (channel, source, channelIndexInAverageBuffer, avgBuffer);
}

void TriggeredAvgCanvas::updateColourForSource (const TriggerSource* source)
{
    m_grid->updateColourForSource (source);
}

void TriggeredAvgCanvas::updateConditionName (const TriggerSource* source)
{
    m_grid->updateConditionName (source);
}

void TriggeredAvgCanvas::prepareToUpdate() { m_grid->prepareToUpdate(); }

void TriggeredAvgCanvas::saveCustomParametersToXml (XmlElement* xml)
{
    m_optionsBar->saveCustomParametersToXml (xml);
}

void TriggeredAvgCanvas::loadCustomParametersFromXml (XmlElement* xml)
{
    m_optionsBar->loadCustomParametersFromXml (xml);
}
