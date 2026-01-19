#pragma once

#include "PluginProcessor.h"
#include "PresetBrowser.h"
#include "VisualizerComponent.h"
#include <JuceHeader.h>

//==============================================================================
//==============================================================================
class DeepCaveLookAndFeel : public juce::LookAndFeel_V4 {
public:
  DeepCaveLookAndFeel();
  void drawWhiteNote(int note, juce::Graphics &g, juce::Rectangle<float> area,
                     bool isDown, bool isOver, juce::Colour lineColour,
                     juce::Colour textColour);
  void drawBlackNote(int note, juce::Graphics &g, juce::Rectangle<float> area,
                     bool isDown, bool isOver, juce::Colour noteFillColour);
  void drawRotarySlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float rotaryStartAngle,
                        float rotaryEndAngle, juce::Slider &slider) override;
  void drawButtonBackground(juce::Graphics &g, juce::Button &button,
                            const juce::Colour &backgroundColour,
                            bool shouldDrawButtonAsHighlighted,
                            bool shouldDrawButtonAsDown) override;
  void drawLinearSlider(juce::Graphics &g, int x, int y, int width, int height,
                        float sliderPos, float minSliderPos, float maxSliderPos,
                        const juce::Slider::SliderStyle,
                        juce::Slider &slider) override;
  // Non-override helper for panels
  void drawPanel(juce::Graphics &g, juce::Rectangle<float> area,
                 const juce::String &title);

  void drawLogo(juce::Graphics &g, juce::Rectangle<float> area);
};

//==============================================================================
/**
 */
class HowlingWolvesAudioProcessorEditor : public juce::AudioProcessorEditor {
public:
  HowlingWolvesAudioProcessorEditor(HowlingWolvesAudioProcessor &);
  ~HowlingWolvesAudioProcessorEditor() override;

  //==============================================================================
  void paint(juce::Graphics &) override;
  void resized() override;

private:
  HowlingWolvesAudioProcessor &audioProcessor;

  // Attachments
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

  // UI Components
  juce::Slider attackSlider;
  juce::Label attackLabel;
  std::unique_ptr<SliderAttachment> attackAttachment;

  juce::Slider decaySlider;
  juce::Label decayLabel;
  std::unique_ptr<SliderAttachment> decayAttachment;

  juce::Slider sustainSlider;
  juce::Label sustainLabel;
  std::unique_ptr<SliderAttachment> sustainAttachment;

  juce::Slider releaseSlider;
  juce::Label releaseLabel;
  std::unique_ptr<SliderAttachment> releaseAttachment;

  juce::Slider gainSlider;
  juce::Label gainLabel;
  std::unique_ptr<SliderAttachment> gainAttachment;

  // Top Bar Buttons
  juce::TextButton browseButton{"BROWSE"};
  juce::TextButton saveButton{"SAVE"};
  juce::TextButton settingsButton{"SETTINGS"};

  juce::MidiKeyboardComponent keyboardComponent;
  DeepCaveLookAndFeel deepCaveLookAndFeel;
  juce::Image backgroundImage;

  // Overlay
  // Overlay
  VisualizerComponent visualizer;
  PresetBrowser presetBrowser;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
      HowlingWolvesAudioProcessorEditor)
};
