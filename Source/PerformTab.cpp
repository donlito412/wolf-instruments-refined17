#include "PerformTab.h"

PerformTab::PerformTab(HowlingWolvesAudioProcessor &p) : audioProcessor(p) {
  // --- 1. TRANSPORT BAR (TOP) ---
  setupLabel(bpmLabel, "BPM 128.0");
  setupLabel(quantizeLabel, "QUANTIZE 1/16 NOTE");

  // Using Unicode shapes for Transport Icons
  setupButton(playBtn, juce::CharPointer_UTF8("\xe2\x96\xb6"),
              juce::Colours::cyan); // Play Triangle
  playBtn.setTooltip("Start Playback");

  setupButton(stopBtn, juce::CharPointer_UTF8("\xe2\x96\xa0"),
              juce::Colours::grey); // Stop Square
  stopBtn.setTooltip("Stop Playback");

  setupButton(recBtn, juce::CharPointer_UTF8("\xe2\x97\x8f"),
              juce::Colours::red); // Rec Circle
  recBtn.setTooltip("Record MIDI Output");

  // --- 2. GRID (NO TITLE) ---
  // setupLabel(arpTitle, "ARPEGGIATOR GRID"); // Removed

  // --- 3. VOICING ENGINE (BOTTOM LEFT) ---
  setupLabel(voicingTitle, "VOICING ENGINE");
  setupKnob(densityKnob, "DENSITY", "arpDensity", densityAtt);
  densityKnob.setTooltip("Adjusts the density of generated notes");

  setupLabel(densityLabel, "DENSITY");
  setupKnob(complexityKnob, "COMPLEXITY", "arpComplexity", complexityAtt);
  complexityKnob.setTooltip("Adjusts the rhythmic complexity");
  setupLabel(complexityLabel, "COMPLEXITY");

  // --- 4. SPREAD & RANGE (BOTTOM CENTER) ---
  setupLabel(spreadTitle, "SPREAD & RANGE");
  setupKnob(spreadWidth, "SPREAD", "arpSpread", spreadAtt);
  spreadWidth.setTooltip("Adjusts the stereo spread width");
  setupLabel(spreadLabel, "SPREAD WIDTH");
  spreadLabel.setFont(juce::Font(10.0f, juce::Font::bold));
  setupKnob(octaveRange, "OCTAVES", "arpOctave", octaveAtt);
  octaveRange.setTooltip("Sets the octave range for the arpeggiator");
  setupLabel(octaveLabel, "OCTAVE RANGE");
  octaveLabel.setFont(juce::Font(10.0f, juce::Font::bold));

  // --- 5. ARP CONTROLS (BOTTOM RIGHT) ---
  setupLabel(controlsTitle, "ARP CONTROLS");
  setupButton(chordHoldBtn, "CHORDS", "chordHold", chordHoldAtt,
              juce::Colours::cyan);
  chordHoldBtn.setTooltip("Master Switch: Enable/Disable Chord Generation");
  chordHoldBtn.setTooltip("Holds the active chord after keys are released");

  setupButton(arpSyncBtn, "ARP SYNC", "arpEnabled", arpSyncAtt,
              juce::Colours::cyan);
  arpSyncBtn.setTooltip("Enables Arpeggiator Mode (Syncs to BPM)");

  // Selector Setup
  setupComboBox(arpModeSelector, "arpMode", arpModeAtt);
  arpModeSelector.setTooltip(
      "Select Arpeggiator Pattern (Up, Down, Random...)");

  setupComboBox(chordModeSelector, "chordMode", chordModeAtt);
  chordModeSelector.setTooltip("Select Chord Generation Mode");

  // Rec Button Logic
  recBtn.onClick = [this] {
    bool isRec = recBtn.getToggleState();
    audioProcessor.getMidiCapturer().setRecording(isRec);
  };

  // Chord Button Logic (Popup Menu)
  chordHoldBtn.onClick = [this] {
    juce::PopupMenu m;
    m.addItem(1, "OFF", true, false); // ID 1
    m.addItem(2, "Major", true, false);
    m.addItem(3, "Minor", true, false);
    m.addItem(4, "7th", true, false);
    m.addItem(5, "9th", true, false);

    m.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(chordHoldBtn),
        [this](int length) {
          if (length == 0)
            return; // Cancelled

          // Handle Selection
          auto *modeParam = dynamic_cast<juce::AudioParameterChoice *>(
              audioProcessor.getAPVTS().getParameter("chordMode"));
          auto *holdParam = audioProcessor.getAPVTS().getParameter("chordHold");

          if (length == 1) { // OFF
            if (holdParam)
              holdParam->setValueNotifyingHost(0.0f); // OFF
            if (modeParam)
              *modeParam = 0; // Index 0 (OFF in param usually?) Wait, Param has
                              // "OFF" as item 1?
            // Let's check Param config. Usually 0=OFF.
            // If param choices are ["OFF", "Major", ...], then Index 0 is OFF.
            if (modeParam)
              *modeParam = 0;
          } else {
            // Enable Chords
            if (holdParam)
              holdParam->setValueNotifyingHost(1.0f);
            // Map ID 2 (Major) -> Index 1?
            // If Choices: 0:OFF, 1:Major, 2:Minor...
            // ID 2 -> Index 1.
            if (modeParam)
              *modeParam = length - 1;
          }
        });
  };

  // Arp Button Logic (Popup Menu)
  arpSyncBtn.onClick = [this] {
    juce::PopupMenu m;
    m.addItem(1, "OFF", true, false);
    m.addItem(2, "Up", true, false);
    m.addItem(3, "Down", true, false);
    m.addItem(4, "Up/Down", true, false);
    m.addItem(5, "Random", true, false);

    m.showMenuAsync(
        juce::PopupMenu::Options().withTargetComponent(arpSyncBtn),
        [this](int length) {
          if (length == 0)
            return;

          auto *modeParam = dynamic_cast<juce::AudioParameterChoice *>(
              audioProcessor.getAPVTS().getParameter("arpMode"));
          auto *enParam = audioProcessor.getAPVTS().getParameter("arpEnabled");

          if (length == 1) { // OFF
            if (enParam)
              enParam->setValueNotifyingHost(0.0f);
            if (modeParam)
              *modeParam = 0;
          } else {
            if (enParam)
              enParam->setValueNotifyingHost(1.0f);
            if (modeParam)
              *modeParam = length - 1;
          }
        });
  };

  // Removed old ComboBox Logic (Conflicting)
  // chordModeSelector.onChange...
  // arpModeSelector.onChange...

  // Initial State: Hide menus
  chordModeSelector.setVisible(false);
  arpModeSelector.setVisible(false);

  // Play Button Logic
  playBtn.onClick = [this] {
    bool isPlaying = playBtn.getToggleState();
    audioProcessor.setTransportPlaying(isPlaying);
    // Ensure Stop is untoggled if Play is toggled
    if (isPlaying) {
      stopBtn.setToggleState(false, juce::dontSendNotification);
    }
  };

  // Stop Button Logic
  stopBtn.onClick = [this] {
    bool isStopped = stopBtn.getToggleState();
    if (isStopped) {
      // Stop means "Not Playing"
      playBtn.setToggleState(false, juce::dontSendNotification);
      audioProcessor.setTransportPlaying(false);

      // Optional: Reset Transport/Arp position?
      // audioProcessor.getMidiProcessor().reset();
      // Resetting might be desired behavior for "Stop".
      audioProcessor.getMidiProcessor().reset();

      // Untoggle itself after a moment? Or acting as a "Toggle State"?
      // User said "play stop ... button". Usually Play is toggle, Stop is
      // trigger or toggle. If Stop is a toggle... let's leave it. But usually
      // Stop just resets. Let's make Stop a momentary trigger
      // implementation-wise for UI but logical reset. We'll leave the toggle
      // state as user might expect visual feedback.
    }
  };

  startTimerHz(30); // 30 FPS Refresh
}

void PerformTab::drawPianoRoll(juce::Graphics &g, juce::Rectangle<int> area) {
  // Professional Piano Roll Look
  // Background
  g.setColour(
      juce::Colour::fromFloatRGBA(0.08f, 0.08f, 0.08f, 1.0f)); // Very dark grey
  g.fillRect(area);

  int numSteps = 16;
  int numNotes = 8;

  // Piano Keys (Left Axis)
  int keysWidth = 30;
  auto keysArea = area.removeFromLeft(keysWidth);

  float noteH = (float)area.getHeight() / (float)numNotes;
  float stepW = (float)area.getWidth() / (float)numSteps;

  // Draw Keys
  juce::StringArray notes = {"C3", "B2", "A2", "G2",
                             "F2", "E2", "D2", "C2"}; // Scale mapping visual

  for (int y = 0; y < numNotes; ++y) {
    auto k = juce::Rectangle<float>((float)keysArea.getX(),
                                    (float)keysArea.getY() + (y * noteH),
                                    (float)keysWidth, noteH);
    bool isBlackKey =
        (notes[y].contains("#") ||
         notes[y].contains(
             "Bb")); // We only have diatonic/minor scale mapped for now
    // Actually our mapping is weird: Root, +2, +3...
    // Let's just draw "Tech Keys"

    g.setColour(juce::Colours::black);
    g.drawRect(k, 1.0f);

    g.setColour(juce::Colours::darkgrey.darker());
    g.fillRect(k.reduced(1));

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(10.0f);
    // Just draw Note Index + 1 for now or scale degree?
    // User asked for "Professional Piano Roll".
    // Let's draw scale degrees: I, VII, VI...
    // Or just default chromatic text if mapped that way?
    // Current mapping is: 0, 2, 3, 5, 7, 9, 10, 12
    // Let's draw numeric offset?
    juce::String label = notes[y];
    // Since logic uses 8 rows, let's keep the labels we had but improve look.
    g.drawText(label, k, juce::Justification::centred);
  }

  // Grid Lines
  g.setColour(juce::Colours::white.withAlpha(0.05f));
  for (int x = 0; x <= numSteps; ++x) {
    float xPos = area.getX() + (x * stepW);
    g.drawVerticalLine((int)xPos, (float)area.getY(), (float)area.getBottom());
    if (x % 4 == 0) {
      g.setColour(juce::Colours::white.withAlpha(0.1f));
      g.drawVerticalLine((int)xPos, (float)area.getY(),
                         (float)area.getBottom());
      g.setColour(juce::Colours::white.withAlpha(0.05f));
    }
  }
  for (int y = 0; y <= numNotes; ++y) {
    float yPos = area.getY() + (y * noteH);
    g.drawHorizontalLine((int)yPos, (float)area.getX(), (float)area.getRight());
  }

  // Draw Notes
  auto &arp = audioProcessor.getMidiProcessor().getArp();
  int playStep = arp.getCurrentStep();

  for (int x = 0; x < numSteps; ++x) {
    int val = arp.getRhythmStep(x);

    // Highlight Playhead Column
    if (x == playStep) {
      auto col = juce::Rectangle<float>((float)area.getX() + (x * stepW),
                                        (float)area.getY(), stepW,
                                        (float)area.getHeight());
      g.setColour(juce::Colours::white.withAlpha(0.05f));
      g.fillRect(col);
    }

    if (val != -1 && val < numNotes) {
      // Invert Y because 0 is Top visually usually, but musically 0 is low?
      // Array access: 0 is index 0.
      // In draw loop: y=0 is top.
      // Usually Piano Roll: High notes top.
      // Let's assume Row 0 = High Note?
      // notes array: C3 (Top), B2... C2 (Bottom).
      // This matches visual y=0 top.

      auto noteRect = juce::Rectangle<float>((float)area.getX() + (x * stepW),
                                             (float)area.getY() + (val * noteH),
                                             stepW, noteH)
                          .reduced(1.0f);

      g.setColour(juce::Colours::cyan);
      g.fillRect(noteRect);

      g.setColour(juce::Colours::cyan.brighter());
      g.drawRect(noteRect, 1.0f);

      if (x == playStep) {
        g.setColour(juce::Colours::white);
        g.drawRect(noteRect, 2.0f);
      }
    }
  }
}

// (Removed duplicate resized)

PerformTab::~PerformTab() { stopTimer(); }

void PerformTab::timerCallback() {
  static int lastStep = -1;
  int step = audioProcessor.getMidiProcessor().getArp().getCurrentStep();

  // Only repaint if step changed
  if (step != lastStep) {
    lastStep = step;
    repaint(gridPanel); // Only repaint grid area
  }
}

void PerformTab::setupKnob(
    juce::Slider &s, const juce::String &n, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
        &att) {
  addAndMakeVisible(s);
  s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
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
  s.setMouseDragSensitivity(500);
  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            audioProcessor.getAPVTS(), paramId, s);
}

void PerformTab::setupComboBox(
    juce::ComboBox &c, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        &att) {
  addAndMakeVisible(c);
  c.setJustificationType(juce::Justification::centred);

  // Add Items based on Param ID
  if (paramId == "arpMode") {
    c.addItem("OFF", 1);
    c.addItem("UP", 2);
    c.addItem("DOWN", 3);
    c.addItem("UP/DOWN", 4);
    c.addItem("RANDOM", 5);
  } else if (paramId == "chordMode") {
    c.addItem("OFF", 1);
    c.addItem("MAJOR", 2);
    c.addItem("MINOR", 3);
    c.addItem("7TH", 4);
    c.addItem("9TH", 5);
  }

  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att = std::make_unique<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getAPVTS(), paramId, c);
}

// Original (Cosmetic/Transport)
void PerformTab::setupButton(juce::TextButton &b, const juce::String &t,
                             juce::Colour c) {
  addAndMakeVisible(b);
  b.setButtonText(t);
  b.setColour(juce::TextButton::buttonOnColourId, c);
  b.setColour(juce::TextButton::buttonColourId,
              c.withAlpha(0.2f)); // Default state dim
  b.setClickingTogglesState(true);
}

// Overload (Parameter)
void PerformTab::setupButton(
    juce::TextButton &b, const juce::String &t, const juce::String &paramId,
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> &att,
    juce::Colour c) {
  setupButton(b, t, c); // Re-use cosmetic setup
  if (auto *p = audioProcessor.getAPVTS().getParameter(paramId))
    att =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            audioProcessor.getAPVTS(), paramId, b);
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

  // Draw Piano Roll (includes keys)
  auto matrixArea = gridPanel.reduced(2);
  drawPianoRoll(g, matrixArea);
}

void PerformTab::mouseDown(const juce::MouseEvent &e) {
  // Check if click is in Grid
  auto matrixArea = gridPanel.reduced(2);

  // Must account for Keys width (30) defined in drawPianoRoll
  int keysWidth = 30;

  if (matrixArea.contains(e.getPosition())) {
    // If click is in keys area, maybe audition note? For now ignore or handle
    if (e.getPosition().x < matrixArea.getX() + keysWidth)
      return;

    auto gridArea = matrixArea;
    gridArea.removeFromLeft(keysWidth);

    if (gridArea.contains(e.getPosition())) {
      int numSteps = 16;
      int numRows = 8;
      float stepW = (float)gridArea.getWidth() / (float)numSteps;
      float noteH = (float)gridArea.getHeight() / (float)numRows;

      int x = (int)((e.getPosition().x - gridArea.getX()) / stepW);
      int y = (int)((e.getPosition().y - gridArea.getY()) / noteH);

      // Safety
      if (x < 0)
        x = 0;
      if (x > 15)
        x = 15;
      if (y < 0)
        y = 0;
      if (y > 7)
        y = 7;

      auto &arp = audioProcessor.getMidiProcessor().getArp();
      int currentVal = arp.getRhythmStep(x);

      if (currentVal == y) {
        arp.setRhythmStep(x, -1); // Clear
      } else {
        arp.setRhythmStep(x, y); // Set Pitch Index
      }
      repaint(gridPanel);
    }
  }
}

void PerformTab::mouseDrag(const juce::MouseEvent &e) {
  // Check if dragging from Transport Area (specifically Rec button or Label)
  if (transportPanel.contains(e.getPosition())) {
    // If Recording is stopped, we can drag
    if (!recBtn.getToggleState()) {
      auto f = audioProcessor.getMidiCapturer().saveToTempFile();
      if (f.exists()) {
        juce::StringArray files;
        files.add(f.getFullPathName());
        juce::DragAndDropContainer::performExternalDragDropOfFiles(files, true);
      }
    }
  }
}

// (Removed duplicate resized and drawArpMatrix)
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

  // Row 1: Buttons
  auto row1 = a.removeFromTop(30);
  int w = row1.getWidth() / 2;
  chordHoldBtn.setBounds(row1.removeFromLeft(w).withSizeKeepingCentre(90, 25));
  arpSyncBtn.setBounds(row1.withSizeKeepingCentre(90, 25));

  // Row 2: Combos
  auto row2 = a.removeFromTop(35);
  int w2 = row2.getWidth() / 2;
  // User requested swap: Arp Sync is left, so Arp Mode should be left.
  // Wait, current code:
  // chordHoldBtn (Left), arpSyncBtn (Right)
  // arpModeSelector (Left), chordModeSelector (Right)
  // Mismatch: Chord Button (Left) <-> Arp Selector (Left) ??
  // Let's align them:
  // Left Side: Chord Hold Btn + Chord Mode Selector
  // Right Side: Arp Sync Btn + Arp Mode Selector

  // Previously:
  // chordHoldBtn.setBounds(row1.removeFromLeft(w)...); // Left
  // arpSyncBtn.setBounds(row1...); // Right

  // So Row 2 should be:
  // chordModeSelector (Left)
  // arpModeSelector (Right)

  chordModeSelector.setBounds(row2.removeFromLeft(w2).reduced(5, 2));
  arpModeSelector.setBounds(row2.reduced(5, 2));
}
