#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    void copyWaveformData(std::vector<float>& dest) const;

    int getDownsampling();

private:
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* gain1 = nullptr;
    std::atomic<float>* ring = nullptr;
    std::atomic<float>* gainStages = nullptr;
    std::atomic<float>* gain2 = nullptr;
    std::atomic<float>* smell = nullptr;
    std::atomic<float>* output = nullptr;
    std::atomic<float>* recycle = nullptr;
    int downsample_debug = 0;

    static constexpr int waveformBufferSize = 512;
    std::vector<float> waveformBuffer;
    std::atomic<int> writePosition{0};
    juce::SpinLock waveformLock;

    double currentSampleRate = 44100.0;
    static constexpr int maxChannels = 2;
    float lpfState[maxChannels] = {0.0f};
    float hpfState[maxChannels] = {0.0f};
    float compEnvelope[maxChannels] = {0.0f};
    double ringModPhase1 = 0.0;

    float eqZ1[maxChannels] = {0.0f};
    float eqZ2[maxChannels] = {0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
