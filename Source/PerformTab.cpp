#include "PerformTab.h"

PerformTab::PerformTab(HowlingWolvesAudioProcessor &p) : audioProcessor(p) {
  // --- 1. TRANSPORT BAR (TOP) ---
  setupLabel(bpmLabel, "BPM 128.0");
  setupLabel(quantizeLabel, "QUANTIZE 1/16 NOTE");

  // Using Unicode shapes for Transport Icons
  setupButton(playBtn, juce::CharPointer_UTF8("\xe2\x96\xb6"),
              juce::Colours::cyan); // Play Triangle
  setupButton(stopBtn, juce::CharPointer_UTF8("\xe2\x96\xa0"),
              juce::Colours::grey); // Stop Square
  setupButton(recBtn, juce::CharPointer_UTF8("\xe2\x97\x8f"),
              juce::Colours::red); // Rec Circle

  // --- 2. ARPEGGIATOR GRID ---
  setupLabel(arpTitle, "ARPEGGIATOR GRID");

  // --- 3. VOICING ENGINE (BOTTOM LEFT) ---
  setupLabel(voicingTitle, "VOICING ENGINE");
  setupKnob(densityKnob, "DENSITY", "arpDensity", densityAtt);
  setupLabel(densityLabel, "DENSITY");
  setupKnob(complexityKnob, "COMPLEXITY", "arpComplexity", complexityAtt);
  setupLabel(complexityLabel, "COMPLEXITY");

  // --- 4. SPREAD & RANGE (BOTTOM CENTER) ---
  setupLabel(spreadTitle, "SPREAD & RANGE");
  setupKnob(spreadWidth, "SPREAD", "arpSpread", spreadAtt);
  setupLabel(spreadLabel, "SPREAD WIDTH");
  spreadLabel.setFont(juce::Font(10.0f, juce::Font::bold));
  setupKnob(octaveRange, "OCTAVES", "arpOctaves", octaveAtt);
  setupLabel(octaveLabel, "OCTAVE RANGE");
  octaveLabel.setFont(juce::Font(10.0f, juce::Font::bold));

  // --- 5. ARP CONTROLS (BOTTOM RIGHT) ---
  setupLabel(controlsTitle, "ARP CONTROLS");
  setupButton(chordHoldBtn, "CHORD HOLD", juce::Colours::cyan);
  setupButton(arpSyncBtn, "ARP SYNC", juce::Colours::cyan);
}

PerformTab::~PerformTab() {}

void PerformTab::setupKnob(
    juce::Slider &s, const juce::String &n, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(juce::Slider::Rotary);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  s.setMouseDragSensitivity(
      500); // Standard is 250, 500 = finer control/less loose
  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void PerformTab::setupSlider(
    juce::Slider &s, const juce::String &n, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(juce::Slider::LinearHorizontal);
  s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void PerformTab::setupButton(juce::TextButton &b, const juce::String &t,
                             juce::Colour c) {
  addAndMakeVisible(b);
  b.setButtonText(t);
  b.setColour(juce::TextButton::buttonOnColourId, c);
  b.setColour(juce::TextButton::buttonColourId,
              c.withAlpha(0.2f)); // Default state dim
  b.setClickingTogglesState(true);
}

void PerformTab::setupLabel(juce::Label &l, const juce::String &t) {
  addAndMakeVisible(l);
  l.setText(t, juce::dontSendNotification);
  l.setColour(juce::Label::textColourId, juce::Colours::silver);
  l.setJustificationType(juce::Justification::centred);
  l.setFont(juce::Font(12.0f, juce::Font::bold));
}

void PerformTab::paint(juce::Graphics &g) {
  auto *lnf = dynamic_cast<ObsidianLookAndFeel *>(&getLookAndFeel());
  if (lnf) {
    // Draw Obsidian Glass Panels
    lnf->drawGlassPanel(g, transportPanel);
    lnf->drawGlassPanel(g, gridPanel);
    lnf->drawGlassPanel(g, voicingPanel);
    lnf->drawGlassPanel(g, spreadPanel);
    lnf->drawGlassPanel(g, controlsPanel);
  }

  // Draw Arp Grid with Note Labels
  // Pass an area that leaves room for labels on the left relative to the grid
  auto matrixArea = gridPanel.reduced(10); // Uniform padding
  matrixArea.removeFromLeft(25);           // Reserve space for labels logic
  drawArpMatrix(g, matrixArea);
}

void PerformTab::drawArpMatrix(juce::Graphics &g, juce::Rectangle<int> area) {
  int numSteps = 16;
  int numNotes = 8;
  float stepW = (float)area.getWidth() / (float)numSteps;
  float noteH = (float)area.getHeight() / (float)numNotes;
  juce::StringArray notes = {"C3", "B2", "A2", "G2", "F2", "E2", "D2", "C2"};

  for (int y = 0; y < numNotes; ++y) {
    // Draw Note Labels on left
    g.setColour(juce::Colours::silver.withAlpha(0.6f));
    g.setFont(10.0f); // Smaller font
    if (y < notes.size())
      g.drawText(notes[y], area.getX() - 28, (int)(area.getY() + (y * noteH)),
                 25, (int)noteH, juce::Justification::centredRight);

    for (int x = 0; x < numSteps; ++x) {
      auto cell =
          juce::Rectangle<float>((float)area.getX() + (x * stepW),
                                 (float)area.getY() + (y * noteH), stepW, noteH)
              .reduced(4.0f);

      // Mock pattern logic from snippet
      bool active = (x % 4 == 0 && (7 - y) == (x / 4));

      g.setColour(active ? juce::Colours::cyan
                         : juce::Colours::black.withAlpha(0.3f));
      g.fillRoundedRectangle(cell, 3.0f);
      if (!active) {
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawRoundedRectangle(cell, 3.0f, 1.0f);
      }
    }
  }
}

void PerformTab::resized() {
  auto area = getLocalBounds().reduced(15);

  // Transport Bar
  transportPanel = area.removeFromTop(50).reduced(5);
  auto tArea = transportPanel.reduced(10);

  int btnW = 50;
  playBtn.setBounds(tArea.removeFromLeft(btnW).reduced(2));
  stopBtn.setBounds(tArea.removeFromLeft(btnW).reduced(2));
  recBtn.setBounds(tArea.removeFromLeft(btnW).reduced(2));

  quantizeLabel.setBounds(tArea.removeFromRight(150));
  bpmLabel.setBounds(tArea.removeFromRight(100));

  // Grid Section
  area.removeFromTop(10); // Spacing
  // Reduced grid height to give more room to bottom modules
  gridPanel = area.removeFromTop((int)(area.getHeight() * 0.55f)).reduced(5);
  arpTitle.setBounds(gridPanel.getX() + 10, gridPanel.getY() + 5, 200, 30);

  // Bottom Modules
  auto bottomArea = area.reduced(0, 10);
  voicingPanel = bottomArea.removeFromLeft((int)(bottomArea.getWidth() * 0.33f))
                     .reduced(5);
  controlsPanel =
      bottomArea.removeFromRight((int)(bottomArea.getWidth() * 0.5f))
          .reduced(5);
  spreadPanel = bottomArea.reduced(5);

  // Layout internal components
  layoutVoicing();
  layoutSpread();
  layoutControls();
}

void PerformTab::layoutVoicing() {
  auto a = voicingPanel.reduced(15);
  voicingTitle.setBounds(a.removeFromTop(25));

  int w = a.getWidth() / 2;

  auto left = a.removeFromLeft(w);
  densityKnob.setBounds(left.withSizeKeepingCentre(45, 45).translated(0, -10));
  densityLabel.setBounds(left.getX(), densityKnob.getBottom(), left.getWidth(),
                         15);

  complexityKnob.setBounds(a.withSizeKeepingCentre(45, 45).translated(0, -10));
  complexityLabel.setBounds(a.getX(), complexityKnob.getBottom(), a.getWidth(),
                            15);
}

void PerformTab::layoutSpread() {
  auto a = spreadPanel.reduced(15);
  spreadTitle.setBounds(a.removeFromTop(25));

  int w = a.getWidth() / 2;

  auto left = a.removeFromLeft(w);
  spreadWidth.setBounds(left.withSizeKeepingCentre(45, 45).translated(0, -10));
  spreadLabel.setBounds(left.getX(), spreadWidth.getBottom(), left.getWidth(),
                        15);

  octaveRange.setBounds(a.withSizeKeepingCentre(45, 45).translated(0, -10));
  octaveLabel.setBounds(a.getX(), octaveRange.getBottom(), a.getWidth(), 15);
}

void PerformTab::layoutControls() {
  auto a = controlsPanel.reduced(15);
  controlsTitle.setBounds(a.removeFromTop(25));

  int w = a.getWidth() / 2;
  chordHoldBtn.setBounds(a.removeFromLeft(w).withSizeKeepingCentre(90, 30));
  arpSyncBtn.setBounds(a.withSizeKeepingCentre(90, 30));
}
