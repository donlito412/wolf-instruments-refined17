#pragma once
#include "ObsidianLookAndFeel.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

class PerformTab : public juce::Component {
public:
  PerformTab(HowlingWolvesAudioProcessor &p);
  ~PerformTab() override;

  void paint(juce::Graphics &g) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent &e) override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // --- Drawing Helpers ---
  void drawArpMatrix(juce::Graphics &g, juce::Rectangle<int> area);

  // --- Layout Helpers ---
  void layoutVoicing();
  void layoutSpread();
  void layoutControls();

  // --- Component Setup ---
  void setupKnob(
      juce::Slider &s, const juce::String &n, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupSlider(
      juce::Slider &s, const juce::String &n, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
          &att);
  void setupButton(juce::TextButton &b, const juce::String &t, juce::Colour c);
  void setupButton(
      juce::TextButton &b, const juce::String &t, const juce::String &paramId,
      std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
          &att,
      juce::Colour c);
  void setupLabel(juce::Label &l, const juce::String &t);

  // --- Panels ---
  juce::Rectangle<int> transportPanel, gridPanel, voicingPanel, spreadPanel,
      controlsPanel;

  // --- Controls ---
  // Transport
  juce::TextButton playBtn, stopBtn, recBtn;
  juce::Label bpmLabel, quantizeLabel;

  // Grid
  juce::Label arpTitle;

  // Bottom Modules
  juce::Label voicingTitle, spreadTitle, controlsTitle;
  juce::Label densityLabel, complexityLabel, spreadLabel, octaveLabel;
  juce::Slider densityKnob, complexityKnob;
  juce::Slider spreadWidth, octaveRange;
  juce::TextButton chordHoldBtn, arpSyncBtn;

  // --- Attachments ---
  // (Placeholders for now as many ARP params might not exist in Processor yet)
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      densityAtt, complexityAtt, spreadAtt, octaveAtt;
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      arpSyncAtt, chordHoldAtt;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PerformTab)
};
