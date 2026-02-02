#pragma once
#include "ObsidianLookAndFeel.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

class ModulateTab : public juce::Component, public juce::Timer {
public:
  ModulateTab(HowlingWolvesAudioProcessor &p);
  ~ModulateTab() override;

  void timerCallback() override;
  void paint(juce::Graphics &g) override;
  void resized() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // --- Helpers ---
  void drawLFOWave(juce::Graphics &g, juce::Rectangle<int> area);
  void setupKnob(
      juce::Slider &s, const juce::String &name, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupSlider(
      juce::Slider &s, const juce::String &name, bool horizontal,
      const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupLabel(juce::Label &l, const juce::String &t);

  // --- Layout Rects (Visual Panels) ---
  // Note: User snippet used Rectangles for panel bounds in paint/resize.
  // I will use Components as containers if I want proper mouse/child
  // management, but the user logic explicitly paints "visPanel" etc which are
  // likely rectangles. However, for consistency with previous tabs and cleaner
  // Z-order, I will use Components. Wait, the User Snippet defines
  // `juce::Rectangle<int> visPanel, lfoPanel, routingPanel;`. So I MUST follow
  // that variable type definition to match the snippet logic exactly.
  juce::Rectangle<int> visPanel, lfoPanel, routingPanel;

  // --- Controls ---
  // LFO
  juce::Slider rateKnob, depthKnob, phaseKnob, smoothSlider;
  juce::ComboBox waveSelector;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAtt,
      depthAtt; // phase/smooth missing in processor currently
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      waveAtt;

  // Routing
  juce::Slider modA, modD, modS, modR, amountSlider;
  juce::ComboBox targetSelector;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
      targetAtt;
  // modADSR and amount missing in processor currently, visuals only for now?

  // Labels
  juce::Label visTitle, syncLabel, lfoTitle, routingTitle;
  juce::Label waveLabel, rateLabel, depthLabel, phaseLabel, smoothLabel;
  juce::Label targetLabel, amountLabel, modALabel, modDLabel, modSLabel,
      modRLabel;

  float phaseOffset = 0.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulateTab)
};
