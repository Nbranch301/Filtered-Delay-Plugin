/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

//==============================================================================
CircularBufferAudioProcessorEditor::CircularBufferAudioProcessorEditor (CircularBufferAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), delayProcessor (p.getDelay())
{
    setSize (600, 400);
    
    // *TITLE & SHADOW*
    //==============================================================================
    static Typeface::Ptr customWoodFont = Typeface::createSystemTypefaceFor(BinaryData::franknplanklight_ttf, BinaryData::franknplanklight_ttfSize);
    woodFont = FontOptions(customWoodFont);
    float titleSize = 47.0f;
    
    pluginTitle.setText("Filtered Delay", dontSendNotification);
    pluginTitle.setFont(FontOptions(woodFont.withHeight(titleSize)));
    pluginTitle.setColour(Label::textColourId, Colour(92, 61, 33));
    pluginTitle.setJustificationType(Justification::centred);

    
    titleShadow.setText("Filtered Delay", dontSendNotification);
    titleShadow.setFont(FontOptions(woodFont.withHeight(titleSize)));
    titleShadow.setColour(Label::textColourId, Colour(80, 49, 21));
    titleShadow.setJustificationType(Justification::centred);
    
    // Show shadow first so it appears behind
    addAndMakeVisible(titleShadow);
    addAndMakeVisible(pluginTitle);

    
    // *DECAY TIME SLIDER*
    //==============================================================================
    decayTimeSlider.setSliderStyle(Slider::RotaryVerticalDrag);
    decayTimeSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    decayTimeSlider.setRange(50.0, 10000.0, 1.0);
    decayTimeSlider.setTextValueSuffix(" ms");
    decayTimeSlider.setLookAndFeel(&knobLAF);
    addAndMakeVisible(decayTimeSlider);
    knobLAF.setLabelForSlider(&decayTimeSlider, "Decay");
    
    decayTimeAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.treeState, PARAM_DECAY_TIME_MS_ID, decayTimeSlider);


    // *DELAY TIME SLIDER*
    //==============================================================================
    delayTimeSlider.setSliderStyle(Slider::RotaryVerticalDrag);
    delayTimeSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    delayTimeSlider.setRange(10.0, 2000.0, 1.0);
    delayTimeSlider.setTextValueSuffix(" ms");
    delayTimeSlider.setLookAndFeel(&knobLAF);
    addAndMakeVisible(delayTimeSlider);
    knobLAF.setLabelForSlider(&delayTimeSlider, "Delay");
    
    delayTimeAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.treeState, PARAM_DELAY_TIME_ID, delayTimeSlider);
    
    // *HIPASS TIME SLIDER*
    //==============================================================================
    hipassSlider.setSliderStyle(Slider::RotaryVerticalDrag);
    hipassSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    hipassSlider.setRange(20.0, 10000.0, 1.0);
    hipassSlider.setTextValueSuffix(" Hz");
    hipassSlider.setLookAndFeel(&knobLAF);
    addAndMakeVisible(hipassSlider);
    knobLAF.setLabelForSlider(&hipassSlider, "HiPass");
    
    hipassAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.treeState, PARAM_HP_CUTOFF_ID, hipassSlider);
    
    // *HIPASS TIME SLIDER*
    //==============================================================================
    lowpassSlider.setSliderStyle(Slider::RotaryVerticalDrag);
    lowpassSlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
    lowpassSlider.setRange(25.0, 20000.0, 1.0);
    lowpassSlider.setTextValueSuffix(" Hz");
    lowpassSlider.setLookAndFeel(&knobLAF);
    addAndMakeVisible(lowpassSlider);
    knobLAF.setLabelForSlider(&lowpassSlider, "LoPass");
    
    lowpassAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.treeState, PARAM_LP_CUTOFF_ID, lowpassSlider);
    
    // *DRY SLIDER*
    //==============================================================================
    drySlider.setSliderStyle(Slider::LinearVertical);
    drySlider.setTextBoxStyle(Slider::TextBoxBelow, false, 80, 20);
    drySlider.setRange(10.0, 2000.0, 1.0);
    drySlider.setTextValueSuffix("%");
    drySlider.setLookAndFeel(&knobLAF);
    addAndMakeVisible(drySlider);
    
    dryLabel.setText("Dry", dontSendNotification);
    dryLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(dryLabel);
    
    dryAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.treeState, PARAM_DRY_ID, drySlider);
    
    // *WET SLIDER*
    //==============================================================================
    wetSlider.setSliderStyle(Slider::LinearVertical);
    wetSlider.setTextBoxStyle(Slider::TextBoxBelow, false, 80, 20);
    wetSlider.setRange(10.0, 2000.0, 1.0);
    wetSlider.setTextValueSuffix("%");
    wetSlider.setLookAndFeel(&knobLAF);
    addAndMakeVisible(wetSlider);
    
    wetLabel.setText("Wet", dontSendNotification);
    wetLabel.setJustificationType(Justification::centred);
    addAndMakeVisible(wetLabel);
    
    wetAttach = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.treeState, PARAM_WET_ID, wetSlider);
}

CircularBufferAudioProcessorEditor::~CircularBufferAudioProcessorEditor()
{
    decayTimeAttach.reset();
    delayTimeAttach.reset();
    hipassAttach.reset();
    lowpassAttach.reset();
    dryAttach.reset();
    wetAttach.reset();
    
    decayTimeSlider.setLookAndFeel(nullptr);
    delayTimeSlider.setLookAndFeel(nullptr);
}

//==============================================================================
void CircularBufferAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (knobLAF.getBackgroundColour());
}

void CircularBufferAudioProcessorEditor::resized()
{
    // Plugin UI Size
    auto bounds = getLocalBounds();
    
    
    // Layout constants
    const int margin = 15;               // outer margin
    const int innerMargin = 10;          // spacing inside right/left panels
    const int rightPanelWidth = getWidth() / 4; // rightmost quarter

    
    // --- Rightmost quarter panel bounds (ie: Area for Wet/Dry Sliders) ---
    auto rightPanel = bounds.removeFromRight(rightPanelWidth)
                            .removeFromTop(3 * (getHeight()/4))
                            .reduced(margin);

    // Split the right panel into two equal columns for both Dry and Wet
    auto dryArea = rightPanel.removeFromLeft(rightPanel.getWidth() / 2).reduced(innerMargin);
    auto wetArea = rightPanel.reduced(innerMargin);

    drySlider.setBounds(dryArea);
    wetSlider.setBounds(wetArea);

    
    // Labels below the sliders:
    const int labelHeight = 18;
    dryLabel.setBounds(dryArea.withTop(dryArea.getBottom() + 4)
                               .withHeight(labelHeight));
    wetLabel.setBounds(wetArea.withTop(wetArea.getBottom() + 4)
                               .withHeight(labelHeight));

    // Left side rotary knobs
    auto leftSide = bounds.reduced(margin);
    auto knobSpacing = 20.0f;
    auto columnSpacing = -20.0f;

    const int knobSize = std::min(leftSide.getWidth() / 3, leftSide.getHeight() / 3);
    auto knobColumn = leftSide.removeFromRight(knobSize).reduced(margin);

    auto decayArea = knobColumn.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize);
    auto delayArea = knobColumn.removeFromTop(knobSize + knobSpacing).withSizeKeepingCentre(knobSize, knobSize);

    decayTimeSlider.setBounds(decayArea);
    delayTimeSlider.setBounds(delayArea);
    
    auto secondKnobColumn = leftSide.removeFromRight(knobSize).translated(columnSpacing, 0).reduced(margin);
    auto hipassArea = secondKnobColumn.removeFromTop(knobSize).withSizeKeepingCentre(knobSize, knobSize);
    auto lowpassArea = secondKnobColumn.removeFromTop(knobSize + knobSpacing).withSizeKeepingCentre(knobSize, knobSize);
    
    hipassSlider.setBounds(hipassArea);
    lowpassSlider.setBounds(lowpassArea);
    
    auto shadowOffset = 1.0f;
    titleShadow.setBounds(margin, margin, hipassSlider.getX() - innerMargin, getHeight()/3);
    pluginTitle.setBounds(titleShadow.getX() - shadowOffset, titleShadow.getY() - shadowOffset, titleShadow.getWidth(), titleShadow.getHeight());
}
