#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"gain1", 1},
            "Gain 1",
            juce::NormalisableRange<float>(0.0f, 40.0f, 0.1f),
            0.0f
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"gain2", 1},
            "Gain 2",
            juce::NormalisableRange<float>(0.0f, 40.0f, 0.1f),
            0.0f
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"garbage", 1},
            "Garbage",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
            0.6f
        ));

        params.push_back(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{"gainStages", 1},
            "Gain Stages",
            0, 7, 1
        ));
        
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"smell", 1},
            "Smell",
            juce::NormalisableRange<float>(-1.0f, 1.0f, 0.01f),
            0.0f
        ));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"output", 1},
            "Output",
            juce::NormalisableRange<float>(-20.0f, 20.0f, 0.01f),
            0.0f
        ));

        return { params.begin(), params.end() };
    }
}

PluginProcessor::PluginProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("BOING"), createParameterLayout())
{
    gain1 = parameters.getRawParameterValue("gain1");
    gain2 = parameters.getRawParameterValue("gain2");
    output = parameters.getRawParameterValue("output");
    ring = parameters.getRawParameterValue("garbage");
    gainStages = parameters.getRawParameterValue("gainStages");
    color = parameters.getRawParameterValue("smell");
    waveformBuffer.resize(waveformBufferSize, 0.0f);
}

PluginProcessor::~PluginProcessor() = default;

const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

void PluginProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        lpfState[ch] = 0.0f;
        hpfState[ch] = 0.0f;
        compEnvelope[ch] = 0.0f;
    }
    ringModPhase1 = 0.0;
    for (int ch = 0; ch < maxChannels; ++ch)
    {
        eqZ1[ch] = 0.0f;
        eqZ2[ch] = 0.0f;
    }
}

void PluginProcessor::releaseResources()
{
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    
    const float gain1Db = gain1->load();
    const float gain1Linear = std::pow(10.0f, gain1Db / 20.0f);
    constexpr float xoverValue = 220.0f;
    constexpr float alphaValue = 0.7f;
    constexpr float blend1Value = 0.7f;
    constexpr float tanhOffsetValue = 0.3f;
    constexpr float tanhScaleValue = 1.3f;
    const float tanhOffset = alphaValue * tanhOffsetValue;
    const float outputDb = output->load();
    const float outputLinear = std::pow(10.0f, outputDb / 20.0f);
    constexpr float blend2Value = 0.5f;
    const int numGainStages = static_cast<int>(gainStages->load());
    const bool hasDistortion = numGainStages > 0;


    // step 1: apply gain
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            channelData[sample] = channelData[sample] * gain1Linear;
        }
    }
    
    // step 2-3: apply harmonics and soft clipping with frequency band splitting
    const float omega = 2.0f * juce::MathConstants<float>::pi * xoverValue / static_cast<float>(currentSampleRate);
    const float g = std::tan(omega * 0.5f);
    const float k = g / (1.0f + g);

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        float& lpState = lpfState[channel];

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto x = channelData[sample];

            float v = (x - lpState) * k;
            float lowBand = v + lpState;
            lpState = lowBand + v;
            float highBand = x - lowBand;

            auto processNonlinear = [&](float in) {
                float y = tanhScaleValue * std::tanh(in + tanhOffset) - 0.206966f;

                float y_ = y;
                for (int i = 0; i < 2; ++i) {
                    if (y_ < -1.0f)
                        y_ = -1.0f;
                    else if (y_ > 1.0f)
                        y_ = 1.0f;
                    else
                        y_ = 1.5f * (y - std::pow(y, 3) / 3.0f);
                }
                
                return y * (1.0f - blend1Value) + y_ * blend1Value;
                
            };

            float processedLow = processNonlinear(lowBand);
            float processedHigh = processNonlinear(highBand);

            channelData[sample] = processedLow + processedHigh;
        }
    }

    // step 4: apply stereo compression and ring mod
    constexpr float attackMs = 47.0f;
    constexpr float releaseMs = 26.5f;
    constexpr float thresholdDb = -10.0f;
    constexpr float ratio = 7.0f;
    
    const float attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(currentSampleRate)));
    const float releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(currentSampleRate)));
    const float thresholdLin = std::pow(10.0f, thresholdDb / 20.0f);

    const float ringModValue = ring->load();
    constexpr double ringModFreq1Value = 58.0;
    const double phaseIncrement1 = ringModFreq1Value / currentSampleRate;

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        float sumAbs = 0.0f;
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            sumAbs += std::abs(buffer.getReadPointer(channel)[sample]);

        if (sumAbs > compEnvelope[0])
            compEnvelope[0] = attackCoeff * compEnvelope[0] + (1.0f - attackCoeff) * sumAbs;
        else
            compEnvelope[0] = releaseCoeff * compEnvelope[0] + (1.0f - releaseCoeff) * sumAbs;

        float gainReduction = 1.0f;
        if (compEnvelope[0] > thresholdLin)
        {
            float overDb = 20.0f * std::log10(compEnvelope[0] / thresholdLin);
            float reducedDb = overDb * (1.0f - 1.0f / ratio);
            gainReduction = std::pow(10.0f, -reducedDb / 20.0f);
        }

        // compute modulator signal for ring mod
        float modulator = static_cast<float>(std::sin(2.0 * juce::MathConstants<double>::pi * ringModPhase1));
        ringModPhase1 += phaseIncrement1 * (1 - gainReduction * 0.5);
        if (ringModPhase1 >= 1.0)
            ringModPhase1 -= 1.0;

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            float* channelData = buffer.getWritePointer(channel);
            float x = channelData[sample];

            // apply ring mod based on compressor envelope
            float ring = x * modulator;
            float ringModBlend = (1-gainReduction) * ringModValue;
            x = x * (1.0f - ringModBlend) + ring * ringModBlend;

            // apply compression
            x = x * (1.0f - blend2Value) + (x * gainReduction) * blend2Value;
            channelData[sample] = x;
        }
    }

    // step 5: parametric EQ
    const float colorValue = color->load();

    // tune eq according to gain2
    const float eqGainDb = 24 * std::tanh(4.0f * colorValue * colorValue);
    const float eqFreqHz = 40 + (1.0f + colorValue) * 1000;
    const float eqQValue = 0.4 + 0.5 * colorValue * colorValue;
    
    const float A = std::pow(10.0f, eqGainDb / 40.0f);
    const float w0 = 2.0f * juce::MathConstants<float>::pi * eqFreqHz / static_cast<float>(currentSampleRate);
    const float cosW0 = std::cos(w0);
    const float sinW0 = std::sin(w0);
    const float alpha = sinW0 / (2.0f * eqQValue);
    
    const float b0 = 1.0f + alpha * A;
    const float b1 = -2.0f * cosW0;
    const float b2 = 1.0f - alpha * A;
    const float a0 = 1.0f + alpha / A;
    const float a1 = -2.0f * cosW0;
    const float a2 = 1.0f - alpha / A;
    
    const float b0n = b0 / a0;
    const float b1n = b1 / a0;
    const float b2n = b2 / a0;
    const float a1n = a1 / a0;
    const float a2n = a2 / a0;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        float& z1 = eqZ1[channel];
        float& z2 = eqZ2[channel];

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float x = channelData[sample];
            float y = b0n * x + z1;
            z1 = b1n * x - a1n * y + z2;
            z2 = b2n * x - a2n * y;
            channelData[sample] = y;
        }
    }
    


    // step 6: apply gain2
    const float gain2Db = gain2->load();
    const float gain2Linear = std::pow(10.0f, gain2Db / 20.0f);

    // step 7: apply distortion
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float x = channelData[sample];

            for (int i = 0; i < numGainStages; ++i) {
                // apply gain2
                if (i == 0) {
                    x *= gain2Linear;
                }
 
                x *= -1;
                if (x < -0.2f) {
                    if (x < -1.86667f)
                        x = -1.86667f;
                    // soft clipping (lower)
                    x = x + 0.3 * std::pow(x + 0.2f, 2);
                }
                else if (x > 0.55f) {
                    // harder clipping (higher)
                    if (x > 1)
                        x = 1;
                    x = x - 3.0f * std::pow(x - 0.55f, 4);
                }
                x *= -1;
            }
            channelData[sample] = x;
        }
    }


    // step n: apply output level
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            channelData[sample] = channelData[sample] * outputLinear;
        }
    }


    if (buffer.getNumChannels() > 0)
    {
        const float* readPtr = buffer.getReadPointer(0);
        int numSamples = buffer.getNumSamples();
        int pos = writePosition.load();
        
        for (int i = 0; i < numSamples; ++i)
        {
            waveformBuffer[pos] = readPtr[i];
            pos = (pos + 1) % waveformBufferSize;
        }
        writePosition.store(pos);
    }
}

void PluginProcessor::copyWaveformData(std::vector<float>& dest) const
{
    dest.resize(waveformBufferSize);
    int pos = writePosition.load();
    for (int i = 0; i < waveformBufferSize; ++i)
    {
        dest[i] = waveformBuffer[(pos + i) % waveformBufferSize];
    }
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
