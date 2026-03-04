#include "PluginEditor.h"

PluginEditor::PluginEditor(PluginProcessor& processor)
    : AudioProcessorEditor(&processor),
      processorRef(processor),
      waveformDisplay(processor)
{
    gain1Slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gain1Slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(gain1Slider);

    gain1Label.setText("Gain 1", juce::dontSendNotification);
    gain1Label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gain1Label);
    gain1Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getParameters(), "gain1", gain1Slider);

    ringSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ringSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(ringSlider);
    ringLabel.setText("Garbage", juce::dontSendNotification);
    ringLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(ringLabel);
    modAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getParameters(), "garbage", ringSlider);


    numDistortionSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    numDistortionSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(numDistortionSlider);
    numDistortionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
    processorRef.getParameters(), "gainStages", numDistortionSlider);
    numDistortionSlider.addListener(this);

    numDistortionLabel.setText("Gain Stages", juce::dontSendNotification);
    numDistortionLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(numDistortionLabel);
    

    gain2Slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    gain2Slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(gain2Slider);
    gain2Label.setText("Gain 2", juce::dontSendNotification);
    gain2Label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(gain2Label);
    gain2Attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getParameters(), "gain2", gain2Slider);
    updateGain2Visibility();


    colorSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    colorSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(colorSlider);
    colorLabel.setText("Smell", juce::dontSendNotification);
    colorLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(colorLabel);
    colorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getParameters(), "smell", colorSlider);

    outputSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
    addAndMakeVisible(outputSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.getParameters(), "output", outputSlider);
    outputLabel.setText("Output", juce::dontSendNotification);
    outputLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputLabel);


    addAndMakeVisible(waveformDisplay);

    setSize(600, 360);
}

PluginEditor::~PluginEditor()
{
    numDistortionSlider.removeListener(this);
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);
}

void PluginEditor::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    auto waveformArea = bounds.removeFromTop(100);
    waveformDisplay.setBounds(waveformArea);

    auto gainPanel = bounds.removeFromLeft(80);
    gain1Label.setBounds(gainPanel.removeFromTop(30));
    gain1Slider.setBounds(gainPanel);

    auto gain2Panel = bounds.removeFromLeft(80);
    gain2Label.setBounds(gain2Panel.removeFromTop(30));
    gain2Slider.setBounds(gain2Panel);

    auto gainStagesPanel = bounds.removeFromLeft(80);
    numDistortionLabel.setBounds(gainStagesPanel.removeFromTop(30));
    numDistortionSlider.setBounds(gainStagesPanel);

    auto modPanel = bounds.removeFromLeft(80);
    ringLabel.setBounds(modPanel.removeFromTop(30));
    ringSlider.setBounds(modPanel);

    auto colorPanel = bounds.removeFromLeft(80);
    colorLabel.setBounds(colorPanel.removeFromTop(30));
    colorSlider.setBounds(colorPanel);

    auto outputPanel = bounds.removeFromLeft(80);
    outputLabel.setBounds(outputPanel.removeFromTop(30));
    outputSlider.setBounds(outputPanel);
}

void PluginEditor::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &numDistortionSlider)
        updateGain2Visibility();
}

void PluginEditor::updateGain2Visibility()
{
    bool visible = static_cast<int>(numDistortionSlider.getValue()) > 0;
    gain2Slider.setVisible(visible);
    gain2Label.setVisible(visible);
}
