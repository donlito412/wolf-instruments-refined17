#pragma once

#include "ModernCyberLookAndFeel.h"
#include "PluginProcessor.h"
#include "PresetBrowser.h"
#include <JuceHeader.h>

//==============================================================================
class PlayTab : public juce::Component {
public:
  PlayTab(HowlingWolvesAudioProcessor &p);
  ~PlayTab() override;

  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // Sidebar removed (moved to PluginEditor overlay)

  // ADSR Section
  juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
  juce::Label attackLabel, decayLabel, sustainLabel, releaseLabel; // Added
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attackAttachment, decayAttachment, sustainAttachment, releaseAttachment;
  juce::Label adsrLabel;

  // Sample Control Section
  juce::Slider startSlider, endSlider;
  juce::Label startLabel, endLabel; // Added
  juce::ToggleButton loopToggle;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      startAttachment, endAttachment;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      loopAttachment;
  juce::Label sampleLabel;

  // Output Section
  juce::Slider gainSlider, panSlider, tuneSlider;
  juce::Label gainLabel, panLabel, tuneLabel; // Added
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      gainAttachment, panAttachment, tuneAttachment;
  juce::Label outputLabel;

  // Helper to setup knobs
  void setupKnob(
      juce::Slider &slider, const juce::String &name,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &attachment,
      const juce::String &paramId);

  // Helper to setup sliders
  void setupSlider(
      juce::Slider &slider, const juce::String &name,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &attachment,
      const juce::String &paramId);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayTab)
};
