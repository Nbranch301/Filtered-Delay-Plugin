/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
using namespace juce;

//==============================================================================

CircularBufferAudioProcessor::CircularBufferAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ),
        treeState(*this, nullptr, "PARAMS", createParameterLayout())
#endif
{
    treeState.state = ValueTree("saveParams");
}

AudioProcessorValueTreeState::ParameterLayout CircularBufferAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    // Delay time (10.0ms to 2000.0ms)
    params.push_back (std::make_unique<AudioParameterFloat>(PARAM_DELAY_TIME_ID,
                                                            "Delay Time (ms)",
                                                            NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 1.0f), 200.0f));

    // Decay time to -60 dB (in ms)
    params.push_back(std::make_unique<AudioParameterFloat>(PARAM_DECAY_TIME_MS_ID,
                                                           "Decay (ms, to -60 dB)",
                                                           NormalisableRange<float>(50.0f, 10000.0f, 1.0f, 1.0f), 2000.0f));


    // Wet & Dry (0% to 100%)
    params.push_back (std::make_unique<AudioParameterFloat>(PARAM_WET_ID,
                                                            "Wet",
                                                            NormalisableRange<float>(0.0f, 100.0f, 1.0f), 35.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(PARAM_DRY_ID,
                                                            "Dry",
                                                            NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f));

    // Hi & Low Pass Filters (HP: 0HZ to 10,000Hz, LP: 25HZ to 20,000Hz)
    params.push_back (std::make_unique<AudioParameterFloat>(PARAM_HP_CUTOFF_ID, "High-Pass (Hz)",
        NormalisableRange<float>(0.0f, 10000.0f, 1.0f, 0.5f), 0.0f));
    
    params.push_back (std::make_unique<AudioParameterFloat>(PARAM_LP_CUTOFF_ID, "Low-Pass (Hz)",
        NormalisableRange<float>(25.0f, 20000.0f, 1.0f, 0.5f), 20000.0f));

    return { params.begin(), params.end() };
}


CircularBufferAudioProcessor::~CircularBufferAudioProcessor()
{
}

//==============================================================================
const String CircularBufferAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CircularBufferAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool CircularBufferAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool CircularBufferAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}


double CircularBufferAudioProcessor::getTailLengthSeconds() const
{
    const float f = delay.getFeedback();
    const float T = delay.getDelayTime();

    if (f <= 0.0f || T <= 0.0f) return 0.0;

    const double k = std::log(0.001) / std::log(std::max(0.0001f, f)); // safety
    return T * k;
}


int CircularBufferAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int CircularBufferAudioProcessor::getCurrentProgram()
{
    return 0;
}

void CircularBufferAudioProcessor::setCurrentProgram (int index)
{
}

const String CircularBufferAudioProcessor::getProgramName (int index)
{
    return {};
}

void CircularBufferAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void DelayEffect::prepare (double sr, int channels, float maxDelayTime)
{
    sampleRate = sr;
    const int delaySamples = (int)(maxDelayTime * sr);
    delayBuffer.setSize(channels, delaySamples);
    delayBuffer.clear();
    
    writePosition = 0;
    
    
    // Prepare filters per channel
    hpFilters.resize(channels);
    lpFilters.resize(channels);

    dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (uint32) 512;
    spec.numChannels = (uint32) channels;

    for (int ch = 0; ch < channels; ++ch)
    {
        hpFilters[ch].reset();
        lpFilters[ch].reset();
        hpFilters[ch].prepare(spec);
        lpFilters[ch].prepare(spec);

        hpFilters[ch].coefficients = dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpCutoff);
        lpFilters[ch].coefficients = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpCutoff);

    }
    filtersPrepared = true;
}

void DelayEffect::setDelayTime (float delayTime)
{
    // delayTime from user will be in ms. Convert to seconds while calculating delayInSamples:
    delayInSamples = (int)(delayTime * 0.001f * sampleRate);
    delayInSamples = jlimit (1, delayBuffer.getNumSamples() - 1, delayInSamples);
}

void DelayEffect::setFeedback (float feedbackAmount)
{
    feedback = jlimit (0.0f, 0.995f, feedbackAmount);
}


void DelayEffect::setWet (float wetAmount)
{
    // wetAmount will be in range 1.0 to 100.0. Convert to fit in range 0.0 to 1.0:
    wet = jlimit(0.0f, 1.0f, wetAmount / 100.0f);
}

void DelayEffect::setDry (float dryAmount)
{
    // dryAmount will be in range 1.0 to 100.0. Convert to fit in range 0.0 to 1.0:
    dry = jlimit(0.0f, 1.0f, dryAmount / 100.0f);
}


void DelayEffect::setHighPassCutoff(float hpHz)
{
    hpCutoff = jlimit(20.0f, 10000.0f, hpHz);
    if (!filtersPrepared || sampleRate <= 0.0) return;

    auto newCoeffs = dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, hpCutoff);
    
    for (auto& hp : hpFilters)
        hp.coefficients = newCoeffs;
}

void DelayEffect::setLowPassCutoff(float lpHz)
{
    lpCutoff = jlimit(25.0f, 20000.0f, lpHz);
    if (!filtersPrepared || sampleRate <= 0.0) return;


    auto newCoeffs = dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, lpCutoff);
    for (auto& lp : lpFilters)
        lp.coefficients = newCoeffs;

}

void DelayEffect::clear()
{
    delayBuffer.clear();
    writePosition = 0;
}


void DelayEffect::process (AudioBuffer<float>& buffer)
{
    const int numSamples      = buffer.getNumSamples();
    const int numChannels     = buffer.getNumChannels();
    const int delayBufferSize = delayBuffer.getNumSamples();

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* outData   = buffer.getWritePointer(channel);
        auto* delayData = delayBuffer.getWritePointer(channel);

        int localWritePos = writePosition;

        // Handle delay sample by sample
        for (int i = 0; i < numSamples; ++i)
        {
            const int readPos = (localWritePos - delayInSamples + delayBufferSize) % delayBufferSize;

            const float in   = outData[i];       // host-provided input
            const float dly  = delayData[readPos];
        
            float dlyWet = dly;
            
            if (filtersPrepared)
            {
                dlyWet = hpFilters[channel].processSample(dlyWet);
                dlyWet = lpFilters[channel].processSample(dlyWet);
            }

            const float out  = dry * in + wet * dlyWet;
            outData[i]       = out;

            float fb = dlyWet * feedback;
            
            // Process the feedback through HP and LP (per channel)
            if (filtersPrepared)
            {
                fb = hpFilters[channel].processSample(fb);
                fb = lpFilters[channel].processSample(fb);
            }

            // Write back: input + filtered feedback
            delayData[localWritePos] = in + fb;


            if (++localWritePos >= delayBufferSize)
                localWritePos = 0;
        }
    }

    writePosition += numSamples;
    writePosition %= delayBuffer.getNumSamples();
}


void CircularBufferAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    ignoreUnused(samplesPerBlock);

    delay.prepare(sampleRate, getTotalNumOutputChannels(), 2.0f); // 2 s max delay

    readAPVTS();
}

void CircularBufferAudioProcessor::releaseResources()
{
    delay.clear();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool CircularBufferAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void CircularBufferAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (int ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    readAPVTS();
    delay.process (buffer);
}

void CircularBufferAudioProcessor::readAPVTS()
{
    // 1) Read delay time (ms) and set it
    const float delayMs = *treeState.getRawParameterValue(PARAM_DELAY_TIME_ID);
    delay.setDelayTime(delayMs);
    
    // 2) Read decay time (ms)
    const float decayMs = *treeState.getRawParameterValue(PARAM_DECAY_TIME_MS_ID);

    // Protect against edge cases
    const float T = delay.getDelayTime();
    const float D = std::max(0.001f, decayMs * 0.001f);

    // Compute linear feedback f so that the tail reaches -60 dB in D seconds:
    // Using: f = 0.001^(T / D) = 10^(-3 * T / D)
    float f = std::pow(0.001f, T / D);

    // Match previous limits
    f = jlimit(0.0f, 0.995f, f);

    delay.setFeedback(f);

    // Wet & Dry parameters:
    delay.setWet(*treeState.getRawParameterValue(PARAM_WET_ID));
    delay.setDry(*treeState.getRawParameterValue(PARAM_DRY_ID));
    
    // Hi & Low pass parameters:
    delay.setHighPassCutoff(*treeState.getRawParameterValue(PARAM_HP_CUTOFF_ID));
    delay.setLowPassCutoff(*treeState.getRawParameterValue(PARAM_LP_CUTOFF_ID));
}

//==============================================================================
bool CircularBufferAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* CircularBufferAudioProcessor::createEditor()
{
    return new CircularBufferAudioProcessorEditor (*this);
}

//==============================================================================
void CircularBufferAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    std::unique_ptr<XmlElement> xml(treeState.state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CircularBufferAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> theParams(getXmlFromBinary(data, sizeInBytes));
    if (theParams != nullptr)
    {
        // If xml has name "saveParams"
        if(theParams -> hasTagName(treeState.state.getType()))
        {
            treeState.state = ValueTree::fromXml(*theParams);
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CircularBufferAudioProcessor();
}
