#include "PlayTab.h"

PlayTab::PlayTab(HowlingWolvesAudioProcessor &p) : audioProcessor(p) {
  // ADSR Section
  setupKnob(attackSlider, "Attack", attackAttachment, "attack");
  setupKnob(decaySlider, "Decay", decayAttachment, "decay");
  setupKnob(sustainSlider, "Sustain", sustainAttachment, "sustain");
  setupKnob(releaseSlider, "Release", releaseAttachment, "release");

  // Init ADSR Labels
  auto initLabel = [this](juce::Label &l, const juce::String &text) {
    addAndMakeVisible(l);
    l.setText(text, juce::dontSendNotification);
    l.setFont(12.0f);
    l.setJustificationType(juce::Justification::centred);
    l.setColour(juce::Label::textColourId, WolfColors::TEXT_SECONDARY);
  };

  initLabel(attackLabel, "A");
  initLabel(decayLabel, "D");
  initLabel(sustainLabel, "S");
  initLabel(releaseLabel, "R");

  addAndMakeVisible(adsrLabel);
  adsrLabel.setText("ENVELOPE", juce::dontSendNotification);
  adsrLabel.setFont(juce::Font(14.0f, juce::Font::bold));
  adsrLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);

  // Sample Section
  // Use setupSlider to connect params
  setupSlider(startSlider, "Start", startAttachment, "sampleStart");
  setupSlider(endSlider, "End", endAttachment, "sampleEnd");

  initLabel(startLabel, "Start");
  initLabel(endLabel, "End");
  startLabel.setJustificationType(juce::Justification::centredRight);
  endLabel.setJustificationType(juce::Justification::centredRight);

  addAndMakeVisible(loopToggle);
  loopToggle.setButtonText("Loop");
  if (audioProcessor.getAPVTS().getParameter("sampleLoop") != nullptr) {
    loopAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getAPVTS(), "sampleLoop", loopToggle);
  }

  addAndMakeVisible(sampleLabel);
  sampleLabel.setText("SAMPLE", juce::dontSendNotification);
  sampleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
  sampleLabel.setColour(juce::Label::textColourId, WolfColors::ACCENT_CYAN);

  // --- Output Section ---
  initLabel(gainLabel, "Gain");
  initLabel(panLabel, "Pan");
  initLabel(tuneLabel, "Tune");
  // Right align labels for linear sliders
  gainLabel.setJustificationType(juce::Justification::centredRight);
  panLabel.setJustificationType(juce::Justification::centredRight);
  tuneLabel.setJustificationType(juce::Justification::centredRight);

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
  slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  addAndMakeVisible(slider);

  if (audioProcessor.getAPVTS().getParameter(paramId) != nullptr) {
    attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, slider);
  }
}

void PlayTab::paint(juce::Graphics &g) {
  auto mainArea = getLocalBounds().removeFromRight(getWidth() - 200);

  g.setColour(WolfColors::PANEL_DARK);
  g.fillRoundedRectangle(mainArea.toFloat().reduced(10), 4.0f);

  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(mainArea.toFloat().reduced(10), 4.0f, 1.0f);
}

// Manual layout for reliability
void PlayTab::resized() {
  auto area = getLocalBounds();
  area.removeFromLeft(200);
  area.reduce(25, 25);

  // 1. ADSR Section
  {
    auto adsrSection = area.removeFromTop(105);
    adsrLabel.setBounds(adsrSection.removeFromTop(20));

    adsrSection.removeFromTop(5); // Spacing

    auto knobArea = adsrSection;
    int knobW = 60;
    int spacing = 10;

    auto layoutAdsrKnob = [&](juce::Slider &s, juce::Label &l) {
      auto slice = knobArea.removeFromLeft(knobW);
      knobArea.removeFromLeft(spacing);

      l.setBounds(slice.removeFromBottom(15));
      s.setBounds(slice);
    };

    layoutAdsrKnob(attackSlider, attackLabel);
    layoutAdsrKnob(decaySlider, decayLabel);
    layoutAdsrKnob(sustainSlider, sustainLabel);
    layoutAdsrKnob(releaseSlider, releaseLabel);
  }

  area.removeFromTop(15); // Gap

  // 2. Sample Section
  {
    auto sampleSection = area.removeFromTop(100);
    sampleLabel.setBounds(sampleSection.removeFromTop(20));
    sampleSection.removeFromTop(5);

    // Start Row
    auto startRow = sampleSection.removeFromTop(24);
    startLabel.setBounds(startRow.removeFromLeft(50));
    startRow.removeFromLeft(5);
    startSlider.setBounds(startRow);

    sampleSection.removeFromTop(5);

    // End Row
    auto endRow = sampleSection.removeFromTop(24);
    endLabel.setBounds(endRow.removeFromLeft(50));
    endRow.removeFromLeft(5);
    endSlider.setBounds(endRow);

    sampleSection.removeFromTop(5);

    // Loop
    loopToggle.setBounds(sampleSection.removeFromTop(20).removeFromLeft(60));
  }

  area.removeFromTop(15);

  // 3. Output Section
  {
    auto outputSection = area.removeFromTop(130);
    outputLabel.setBounds(outputSection.removeFromTop(20));
    outputSection.removeFromTop(5);

    auto layoutOutputRow = [&](juce::Slider &s, juce::Label &l) {
      auto row = outputSection.removeFromTop(24);
      l.setBounds(row.removeFromLeft(50));
      row.removeFromLeft(5);
      s.setBounds(row);
      outputSection.removeFromTop(5);
    };

    layoutOutputRow(gainSlider, gainLabel);
    layoutOutputRow(panSlider, panLabel);
    layoutOutputRow(tuneSlider, tuneLabel);
  }
}
