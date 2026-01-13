/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#define PARAM_DELAY_TIME_ID "delayTime"
#define PARAM_DECAY_TIME_MS_ID "decayTimeMs"
#define PARAM_WET_ID "wet"
#define PARAM_DRY_ID "dry"
#define PARAM_HP_CUTOFF_ID "hipass"
#define PARAM_LP_CUTOFF_ID "lowpass"

using namespace juce;

//==============================================================================
/**
*/

class DelayEffect
{
public:
    void prepare (double sr, int channels, float maxDelayTime);
    
    void setDelayTime (float delayTime);
    void setFeedback (float feedbackAmount);
    void setWet (float wetAmount);
    void setDry (float dryAmount);
    void setHighPassCutoff(float hpHz);
    void setLowPassCutoff(float lpHz);
    
    void clear();
    void process (AudioBuffer<float>& buffer);
    
    float getDelayTime() const { return delayInSamples / static_cast<float>(sampleRate); }
    float getFeedback()  const { return feedback; }

private:
    AudioBuffer<float> delayBuffer;
    int writePosition   = 0;
    double sampleRate   = 44100.0;
    int delayInSamples  = 0;
    
    float feedback  = 0.5f;
    float wet       = 50.0;
    float dry       = 100.0;
    
    std::vector<dsp::IIR::Filter<float>> hpFilters;
    std::vector<dsp::IIR::Filter<float>> lpFilters;
    float hpCutoff = 60.0f;
    float lpCutoff = 8000.0f;
    bool filtersPrepared = false;
};

class CircularBufferAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    CircularBufferAudioProcessor();
    ~CircularBufferAudioProcessor() override;
    
    AudioProcessorValueTreeState treeState;
    DelayEffect& getDelay() noexcept { return delay; }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    void readAPVTS();

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    DelayEffect delay;
    
    static AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircularBufferAudioProcessor)
};
