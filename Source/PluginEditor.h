/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

using namespace juce;

//==============================================================================
/**
*/

class KnobLookAndFeel : public LookAndFeel_V4
{
public:
    
    // Colour Palette
    Colour trackFillClr = Colour(85, 111, 59);
    Colour trackBackgroundClr = Colour(102, 71, 43);
    Colour outerBackgroundClr = Colour(90, 59, 31);
    Colour thumbClr = Colour(203, 185, 157);
    Colour backgroundClr = Colour(139, 99, 65);
    
    
    KnobLookAndFeel()
    {
        setColour(Slider::thumbColourId, thumbClr);
        setColour(Slider::backgroundColourId, trackBackgroundClr);
        setColour(Slider::trackColourId, trackFillClr);
        setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
    }
    
    Colour getBackgroundColour(){
        return backgroundClr;
    }

    void setLabelForSlider(const Slider* slider, const String& labelText)
    {
        labels.set(slider, labelText);
    }

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle, float rotaryEndAngle,
                          Slider& slider) override
    {
        auto bounds = Rectangle<int>(x, y, width, height).toFloat();
        auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f - 2.0f;
        auto centre = bounds.getCentre();
        auto circleMargin = 5.0f;
        
        // Outer Background circle
        g.setColour(outerBackgroundClr);
        g.fillEllipse(centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
        
        // Inner Background circle
        g.setColour(trackBackgroundClr);
        g.fillEllipse(centre.x - radius + circleMargin / 2.0f, centre.y - radius + circleMargin / 2.0f, radius * 2.0f - circleMargin, radius * 2.0f - circleMargin);
        
        // Dial Track
        auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        {
            Path arc;
            arc.addCentredArc(centre.x, centre.y, radius - 6.0f, radius - 6.0f,
                              0.0f, rotaryStartAngle, angle, true);
            
            g.strokePath(arc, PathStrokeType(3.0f));

            Path arcValue;
            arcValue.addCentredArc(centre.x, centre.y, radius - 6.0f, radius - 6.0f,
                                   0.0f, rotaryStartAngle, angle, true);
            g.setColour(trackFillClr);
            g.strokePath(arcValue, PathStrokeType(3.0f));
        }
        
        // Dial pointer
        {
            const float pointerLen = radius - 10.0f;
            const float pointerThickness = 3.0f;
            Path p;
            p.addRectangle(-pointerThickness * 0.5f, -pointerLen, pointerThickness, pointerLen);
            p.applyTransform(AffineTransform::rotation(angle).translated(centre.x, centre.y));
            g.setColour(thumbClr);
            g.fillPath(p);
        }

        // Centered labels
        const auto textYOffset = radius - 35.0f;
        const auto labelTextDist = 15.0f;
        const auto labelText = labels.contains(&slider) ? labels[&slider] : String();
        auto labelArea = bounds.withSizeKeepingCentre(bounds.getWidth() * 0.8f, 20.0f);
        
        if (labelText.isNotEmpty())
        {
            g.setColour(Colours::white.withAlpha(0.9f));

            labelArea.setY((int)(centre.y + textYOffset));
            g.drawFittedText(labelText, labelArea.toNearestInt(), Justification::centred, 1);
        }

        // Numeric value
        {
            g.setColour(Colours::white);
            
            String valueText;
            // Use suffix if set on the slider, otherwise plain number
            const auto val = slider.getValue();
            auto suffix = slider.getTextValueSuffix();
            if (suffix.isNotEmpty())
                valueText = String((int)std::round(val)) + " " + suffix.trim();
            else
                valueText = String(val, 2);

            auto valueArea = bounds.withSizeKeepingCentre(bounds.getWidth() * 0.8f, 20.0f);
            valueArea.setY((int)(labelArea.getY() + labelTextDist)); // slightly below the label
            g.drawFittedText(valueText, valueArea.toNearestInt(), Justification::centred, 1);
        }
    }

private:
    HashMap<const Slider*, String> labels;
};

class CircularBufferAudioProcessorEditor  : public AudioProcessorEditor
{
public:
    CircularBufferAudioProcessorEditor (CircularBufferAudioProcessor&);
    ~CircularBufferAudioProcessorEditor() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    
private:
    CircularBufferAudioProcessor& audioProcessor;
    
    FontOptions woodFont;
    
    DelayEffect& delayProcessor;
    
    Slider  decayTimeSlider,
            delayTimeSlider,
            hipassSlider,
            lowpassSlider,
            drySlider,
            wetSlider;
    
    Label   decayTimeLabel,
            delayTimeLabel,
            dryLabel,
            wetLabel,
            pluginTitle,
            titleShadow;
    
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> decayTimeAttach,
                                                                    delayTimeAttach,
                                                                    hipassAttach,
                                                                    lowpassAttach,
                                                                    dryAttach,
                                                                    wetAttach;
    KnobLookAndFeel knobLAF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircularBufferAudioProcessorEditor)
};
