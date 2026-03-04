#pragma once

#include "PluginProcessor.h"

class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    explicit WaveformDisplay(PluginProcessor& p) : processor(p)
    {
        startTimerHz(30);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::green);

        processor.copyWaveformData(localBuffer);
        if (localBuffer.empty()) return;

        juce::Path path;
        float width = static_cast<float>(getWidth());
        float height = static_cast<float>(getHeight());
        float midY = height * 0.5f;

        path.startNewSubPath(0, midY);
        for (size_t i = 0; i < localBuffer.size(); ++i)
        {
            float x = (static_cast<float>(i) / static_cast<float>(localBuffer.size())) * width;
            float y = midY - (localBuffer[i] * midY);
            path.lineTo(x, y);
        }

        g.strokePath(path, juce::PathStrokeType(1.5f));
    }

private:
    void timerCallback() override { repaint(); }
    PluginProcessor& processor;
    std::vector<float> localBuffer;
};

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Slider::Listener
{
public:
    explicit PluginEditor(PluginProcessor& processor);
    ~PluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void sliderValueChanged(juce::Slider* slider) override;
    void updateGain2Visibility();

    PluginProcessor& processorRef;

    juce::Slider gain1Slider;
    juce::Label gain1Label;

    juce::Slider ringSlider;
    juce::Label ringLabel;

    juce::Slider numDistortionSlider;
    juce::Label numDistortionLabel;

    juce::Slider gain2Slider;
    juce::Label gain2Label;

    juce::Slider colorSlider;
    juce::Label colorLabel;

    juce::Slider outputSlider;
    juce::Label outputLabel;
    
    WaveformDisplay waveformDisplay;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gain1Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> numDistortionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gain2Attachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> colorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};
