#include "PlayTab.h"

PlayTab::PlayTab(HowlingWolvesAudioProcessor &p)
    : audioProcessor(p), presetBrowser(p.getPresetManager()) {
  // Sidebar
  addAndMakeVisible(presetBrowser);

  // ADSR Section
  setupKnob(attackSlider, "A", attackAttachment, "attack");
  setupKnob(decaySlider, "D", decayAttachment, "decay");
  setupKnob(sustainSlider, "S", sustainAttachment, "sustain");
  setupKnob(releaseSlider, "R", releaseAttachment, "release");

  addAndMakeVisible(adsrLabel);
  adsrLabel.setText("ENVELOPE", juce::dontSendNotification);
  adsrLabel.setFont(juce::Font(14.0f, juce::Font::bold));
  adsrLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);

  // Sample Section
  startSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  startSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  // No params attached yet for sample start/end, just visuals for Phase 2
  addAndMakeVisible(startSlider);
  addAndMakeVisible(endSlider);
  endSlider.setSliderStyle(juce::Slider::LinearHorizontal);
  endSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

  addAndMakeVisible(loopToggle);
  loopToggle.setButtonText("Loop");

  addAndMakeVisible(sampleLabel);
  sampleLabel.setText("SAMPLE", juce::dontSendNotification);
  sampleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
  sampleLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);

  // Output Section
  setupSlider(gainSlider, "Gain", gainAttachment, "gain");
  setupSlider(panSlider, "Pan", panAttachment, "pan");
  setupSlider(tuneSlider, "Tune", tuneAttachment, "tune");

  addAndMakeVisible(outputLabel);
  outputLabel.setText("OUTPUT", juce::dontSendNotification);
  outputLabel.setFont(juce::Font(14.0f, juce::Font::bold));
  outputLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);
}

PlayTab::~PlayTab() {}

void PlayTab::setupKnob(
    juce::Slider &slider, const juce::String &name,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &attachment,
    const juce::String &paramId) {
  slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
  slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  addAndMakeVisible(slider);

  // Try to attach if parameter exists
  if (audioProcessor.getAPVTS().getParameter(paramId) != nullptr) {
    attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, slider);
  }
}

void PlayTab::setupSlider(
    juce::Slider &slider, const juce::String &name,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &attachment,
    const juce::String &paramId) {
  slider.setSliderStyle(juce::Slider::LinearHorizontal);
  slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
  addAndMakeVisible(slider);

  if (audioProcessor.getAPVTS().getParameter(paramId) != nullptr) {
    attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, slider);
  }
}

void PlayTab::paint(juce::Graphics &g) {
  // Main area background (semi-transparent panel over cave)
  auto mainArea = getLocalBounds().removeFromRight(getWidth() - 200);

  // Draw semi-transparent panel for Sound Engine
  g.setColour(WolfColors::PANEL_DARK);
  g.fillRoundedRectangle(mainArea.toFloat().reduced(10), 4.0f);

  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(mainArea.toFloat().reduced(10), 4.0f, 1.0f);

  // Draw Section Dividers/Headers?
  // Handled by resized() placement mostly
}

void PlayTab::resized() {
  auto area = getLocalBounds();

  // Sidebar (200px)
  presetBrowser.setBounds(area.removeFromLeft(200));

  // Main Content
  area.reduce(25, 25); // Padding inside the panel

  juce::FlexBox mainLayout;
  mainLayout.flexDirection = juce::FlexBox::Direction::column;
  mainLayout.justifyContent = juce::FlexBox::JustifyContent::flexStart;
  mainLayout.alignContent = juce::FlexBox::AlignContent::stretch;

  // ADSR Section
  juce::FlexBox adsrBox;
  adsrBox.flexDirection = juce::FlexBox::Direction::row;
  adsrBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
  adsrBox.items.add(juce::FlexItem(attackSlider)
                        .withWidth(60)
                        .withHeight(60)
                        .withMargin({0, 10, 0, 0}));
  adsrBox.items.add(juce::FlexItem(decaySlider)
                        .withWidth(60)
                        .withHeight(60)
                        .withMargin({0, 10, 0, 0}));
  adsrBox.items.add(juce::FlexItem(sustainSlider)
                        .withWidth(60)
                        .withHeight(60)
                        .withMargin({0, 10, 0, 0}));
  adsrBox.items.add(juce::FlexItem(releaseSlider).withWidth(60).withHeight(60));

  mainLayout.items.add(juce::FlexItem(adsrLabel).withHeight(20));
  mainLayout.items.add(
      juce::FlexItem(adsrBox).withHeight(70).withMargin({0, 0, 20, 0}));

  // Sample Section
  mainLayout.items.add(juce::FlexItem(sampleLabel).withHeight(20));
  mainLayout.items.add(juce::FlexItem(startSlider)
                           .withHeight(20)
                           .withFlex(1)
                           .withMargin({0, 0, 5, 0}));
  mainLayout.items.add(
      juce::FlexItem(endSlider).withHeight(20).withFlex(1).withMargin(
          {0, 0, 5, 0}));
  mainLayout.items.add(juce::FlexItem(loopToggle)
                           .withHeight(20)
                           .withWidth(60)
                           .withMargin({0, 0, 20, 0}));

  // Output Section
  mainLayout.items.add(juce::FlexItem(outputLabel).withHeight(20));
  mainLayout.items.add(juce::FlexItem(gainSlider)
                           .withHeight(24)
                           .withFlex(1)
                           .withMargin({0, 0, 5, 0}));
  mainLayout.items.add(
      juce::FlexItem(panSlider).withHeight(24).withFlex(1).withMargin(
          {0, 0, 5, 0}));
  mainLayout.items.add(juce::FlexItem(tuneSlider).withHeight(24).withFlex(1));

  mainLayout.performLayout(area);
}
