#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "VisualizerComponent.h"

//==============================================================================
DeepCaveLookAndFeel::DeepCaveLookAndFeel() {}

void DeepCaveLookAndFeel::drawWhiteNote(int note, juce::Graphics &g,
                                        juce::Rectangle<float> area,
                                        bool isDown, bool isOver,
                                        juce::Colour lineColour,
                                        juce::Colour textColour) {
  auto c = juce::Colour::fromString("FF2A2A30"); // Dark Slate/Charcoal

  if (isDown)
    c = juce::Colour::fromString("FF88CCFF"); // Ice Blue Glow

  // Gradient for depth
  g.setGradientFill(juce::ColourGradient(c.brighter(0.1f), area.getTopLeft(),
                                         c.darker(0.1f), area.getBottomLeft(),
                                         false));
  g.fillRect(area);

  // Silver Edge
  g.setColour(juce::Colour::fromString("FF888890"));
  g.drawRect(area, 1.0f);

  // Bottom shadow simulation
  g.setGradientFill(juce::ColourGradient(
      juce::Colours::black.withAlpha(0.0f),
      area.getBottomLeft().translated(0, -10),
      juce::Colours::black.withAlpha(0.5f), area.getBottomLeft(), false));
  g.fillRect(area.removeFromBottom(10));
}

void DeepCaveLookAndFeel::drawBlackNote(int note, juce::Graphics &g,
                                        juce::Rectangle<float> area,
                                        bool isDown, bool isOver,
                                        juce::Colour noteFillColour) {
  auto c = juce::Colour::fromString("FF101010"); // Near black matte

  if (isDown)
    c = juce::Colour::fromString("FF4477AA"); // Darker Ice Blue

  g.setColour(c);
  g.fillRect(area);

  // Silver Edge
  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRect(area, 1.0f);

  g.setGradientFill(juce::ColourGradient(
      juce::Colours::white.withAlpha(0.1f), area.getTopLeft(),
      juce::Colours::transparentWhite, area.getBottomLeft(), false));
  g.fillRect(area);
}

void DeepCaveLookAndFeel::drawRotarySlider(
    juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) {
  auto radius = (float)juce::jmin(width / 2, height / 2) - 4.0f;
  auto centreX = (float)x + (float)width * 0.5f;
  auto centreY = (float)y + (float)height * 0.5f;
  auto rx = centreX - radius;
  auto ry = centreY - radius;
  auto rw = radius * 2.0f;
  auto angle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

  // Brushed Silver Face (Radial Gradient)
  juce::ColourGradient gradient(juce::Colour::fromString("FFCCCCCC"), centreX,
                                centreY, juce::Colour::fromString("FF333333"),
                                rx, ry, true);
  gradient.addColour(0.4, juce::Colour::fromString("FF888888"));
  gradient.addColour(0.7, juce::Colour::fromString("FFAAAAAA"));

  g.setGradientFill(gradient);
  g.fillEllipse(rx, ry, rw, rw);

  // Metallic Rim
  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawEllipse(rx, ry, rw, rw, 2.0f);

  // Indicator (Ice Blue)
  juce::Path p;
  auto pointerLength = radius * 0.7f;
  auto pointerThickness = 3.0f;
  p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness,
                 pointerLength);
  p.applyTransform(
      juce::AffineTransform::rotation(angle).translated(centreX, centreY));

  g.setColour(juce::Colour::fromString("FF88CCFF")); // Ice Blue
  g.fillPath(p);

  // Glow effect for indicator
  g.setColour(juce::Colour::fromString("FF88CCFF").withAlpha(0.6f));
  g.strokePath(p, juce::PathStrokeType(2.0f));
}

void DeepCaveLookAndFeel::drawButtonBackground(
    juce::Graphics &g, juce::Button &button,
    const juce::Colour &backgroundColour, bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown) {
  auto area = button.getLocalBounds().toFloat();

  // 1. Panel Body (Glass)
  // Brighter on hover (0.5f), standard (0.3f)
  float alpha = shouldDrawButtonAsHighlighted ? 0.5f : 0.3f;
  g.setColour(juce::Colours::black.withAlpha(alpha));
  g.fillRect(area);

  // Active State (Ice Blue Tint)
  if (shouldDrawButtonAsDown) {
    g.setColour(juce::Colour::fromString("FF88CCFF").withAlpha(0.2f));
    g.fillRect(area);
  }

  // 2. Metallic Edge
  // Active = Ice Blue Border, Normal = Metallic
  g.setColour(shouldDrawButtonAsDown ? juce::Colour::fromString("FF88CCFF")
                                     : juce::Colour::fromString("FF666670"));
  g.drawRect(area, 1.0f);

  // 3. Corner Screws (Industrial Feel)
  float screwSize = 3.0f;
  g.setColour(juce::Colour::fromString("FFCCCCCC")); // Silver screws
  // Inset screws slightly
  g.fillEllipse(area.getX() + 3, area.getY() + 3, screwSize, screwSize);
  g.fillEllipse(area.getRight() - 6, area.getY() + 3, screwSize, screwSize);
  g.fillEllipse(area.getX() + 3, area.getBottom() - 6, screwSize, screwSize);
  g.fillEllipse(area.getRight() - 6, area.getBottom() - 6, screwSize,
                screwSize);
}

void DeepCaveLookAndFeel::drawLinearSlider(juce::Graphics &g, int x, int y,
                                           int width, int height,
                                           float sliderPos, float minSliderPos,
                                           float maxSliderPos,
                                           const juce::Slider::SliderStyle,
                                           juce::Slider &slider) {
  auto trackWidth = 4.0f;
  juce::Point<float> startPoint((float)x + (float)width * 0.5f, (float)height);
  juce::Point<float> endPoint((float)x + (float)width * 0.5f, (float)y);

  // Rail
  juce::Path track;
  track.startNewSubPath(startPoint);
  track.lineTo(endPoint);
  g.setColour(juce::Colour::fromString("FF222222"));
  g.strokePath(track, juce::PathStrokeType(trackWidth));

  // Active Rail (Ice Blue)
  juce::Path activeTrack;
  juce::Point<float> sliderPoint((float)x + (float)width * 0.5f, sliderPos);
  activeTrack.startNewSubPath(startPoint);
  activeTrack.lineTo(sliderPoint);
  g.setColour(juce::Colour::fromString("FF88CCFF"));
  g.strokePath(activeTrack, juce::PathStrokeType(trackWidth));

  // Thumb
  g.setColour(juce::Colours::white);
  g.fillEllipse(sliderPoint.x - 5, sliderPoint.y - 5, 10, 10);
}

void DeepCaveLookAndFeel::drawPanel(juce::Graphics &g,
                                    juce::Rectangle<float> area,
                                    const juce::String &title) {
  // Drop Shadow
  g.setColour(juce::Colours::black.withAlpha(0.6f));
  g.fillRect(area.translated(4, 4));

  // Panel Body (Glass / Semi-Transparent)
  g.setColour(juce::Colours::black.withAlpha(0.3f)); // 30% Opacity
  g.fillRect(area);

  // Metallic Edge (Thinner)
  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRect(area, 1.0f);

  // Corner Screws (Smaller)
  float screwSize = 3.0f;
  g.setColour(juce::Colour::fromString("FFCCCCCC"));
  g.fillEllipse(area.getX() + 4, area.getY() + 4, screwSize, screwSize);
  g.fillEllipse(area.getRight() - 8, area.getY() + 4, screwSize, screwSize);
  g.fillEllipse(area.getX() + 4, area.getBottom() - 8, screwSize, screwSize);
  g.fillEllipse(area.getRight() - 8, area.getBottom() - 8, screwSize,
                screwSize);

  // Title Label Background (Darker, transparent)
  if (title.isNotEmpty()) {
    auto titleArea = area.removeFromTop(20);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRect(titleArea);
    g.setColour(juce::Colour::fromString("FF9999AA"));
    g.setFont(12.0f);
    g.drawText(title.toUpperCase(), titleArea, juce::Justification::centred,
               false);
  }
}

void DeepCaveLookAndFeel::drawLogo(juce::Graphics &g,
                                   juce::Rectangle<float> area) {
  // 1. Create Text Path Directly
  juce::Font logoFont(24.0f, juce::Font::bold);
  logoFont.setExtraKerningFactor(0.15f); // Cinematic spacing

  juce::Path textPath;
  juce::GlyphArrangement glyphs;
  glyphs.addLineOfText(logoFont, "HOWLING WOLVES", 0, 0);
  glyphs.createPath(textPath);

  // Center the path in the area
  auto pathBounds = textPath.getBounds();
  auto offset = area.getCentre() - pathBounds.getCentre();
  textPath.applyTransform(
      juce::AffineTransform::translation(offset.x, offset.y));

  // 2. Glow / Backlight (Ice Blue)
  g.setColour(juce::Colour::fromString("FF88CCFF").withAlpha(0.3f));
  for (int i = 0; i < 3; ++i) {
    g.strokePath(textPath, juce::PathStrokeType(6.0f - i * 1.5f));
  }

  // 3. Metallic Fill (Gradient)
  // Recalculate bounds after transform
  pathBounds = textPath.getBounds();
  juce::ColourGradient metalGradient(
      juce::Colour::fromString("FFEEEEEE"), 0, pathBounds.getY(),
      juce::Colour::fromString("FF444444"), 0, pathBounds.getBottom(), false);

  g.setGradientFill(metalGradient);
  g.fillPath(textPath);

  // 4. White Rim for sharpness
  g.setColour(juce::Colours::white.withAlpha(0.4f));
  g.strokePath(textPath, juce::PathStrokeType(1.0f));
}
//==============================================================================
HowlingWolvesAudioProcessorEditor::HowlingWolvesAudioProcessorEditor(
    HowlingWolvesAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      attackAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), "ATTACK", attackSlider)),
      decayAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), "DECAY", decaySlider)),
      sustainAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), "SUSTAIN", sustainSlider)),
      releaseAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), "RELEASE", releaseSlider)),
      gainAttachment(std::make_unique<SliderAttachment>(
          audioProcessor.getAPVTS(), "GAIN", gainSlider)),
      keyboardComponent(audioProcessor.getKeyboardState(),
                        juce::MidiKeyboardComponent::horizontalKeyboard),
      presetBrowser(audioProcessor.getPresetManager()) {

  // Hook Visualizer
  addAndMakeVisible(visualizer);
  audioProcessor.audioVisualizerHook =
      [this](const juce::AudioBuffer<float> &buffer) {
        visualizer.pushBuffer(buffer);
      };

  // Load Background
  backgroundImage = juce::ImageCache::getFromMemory(
      BinaryData::background_png, BinaryData::background_pngSize);

  // Setup Keyboard
  addAndMakeVisible(keyboardComponent);
  keyboardComponent.setAvailableRange(24, 96);
  keyboardComponent.setKeyWidth(20); // Reduced from 30 to 20 ("Skinnier")
  keyboardComponent.setColour(
      juce::MidiKeyboardComponent::keyDownOverlayColourId,
      juce::Colour::fromString("FF88CCFF")); // Ice Blue key press
  keyboardComponent.setColour(
      juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId,
      juce::Colour::fromString("FF88CCFF").withAlpha(0.5f));

  // Initialize UI (See implementation plan)
  deepCaveLookAndFeel.setColour(juce::Slider::thumbColourId,
                                juce::Colour::fromString("FFB0B0B0"));

  // Common Slider Setup Helper
  auto setupSlider = [this](juce::Slider &slider, juce::Label &label,
                            const juce::String &name) {
    addAndMakeVisible(slider);
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setLookAndFeel(&deepCaveLookAndFeel);

    addAndMakeVisible(label);
    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false); // Label above slider
    label.setFont(juce::Font(14.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId,
                    juce::Colours::white.withAlpha(0.8f));
  };

  setupSlider(attackSlider, attackLabel, "ATTACK");
  setupSlider(decaySlider, decayLabel, "DECAY");
  setupSlider(sustainSlider, sustainLabel, "SUSTAIN");
  setupSlider(releaseSlider, releaseLabel, "RELEASE");
  setupSlider(gainSlider, gainLabel, "GAIN");

  // Setup Keyboard Look
  keyboardComponent.setLookAndFeel(&deepCaveLookAndFeel);
  keyboardComponent.setBlackNoteLengthProportion(0.6f);

  // Make plugin resizable
  setResizable(true, true);
  setResizeLimits(600, 400, 1200, 800);
  setSize(800, 600);

  // Top Bar Buttons
  addAndMakeVisible(browseButton);
  addAndMakeVisible(saveButton);
  addAndMakeVisible(settingsButton);

  browseButton.setLookAndFeel(&deepCaveLookAndFeel);
  saveButton.setLookAndFeel(&deepCaveLookAndFeel);
  settingsButton.setLookAndFeel(&deepCaveLookAndFeel);

  // Button Callbacks
  browseButton.onClick = [this] {
    // Toggle Browser
    bool isVisible = presetBrowser.isVisible();
    presetBrowser.setVisible(!isVisible);
    if (!isVisible) {
      presetBrowser.toFront(true);
      presetBrowser.refresh();
    }
  };

  saveButton.onClick = [this] {
    // Quick Save (Demo)
    audioProcessor.getPresetManager().savePreset(
        "New Preset " + juce::Time::getCurrentTime().toString(true, true));
  };

  // Browser (Always on top)
  presetBrowser.onPresetSelected = [this] {
    presetBrowser.setVisible(false);
    repaint(); // Update LCD
  };
  addChildComponent(presetBrowser);
  presetBrowser.setVisible(false); // Hidden by default
}

HowlingWolvesAudioProcessorEditor::~HowlingWolvesAudioProcessorEditor() {
  audioProcessor.audioVisualizerHook = nullptr;
}

void HowlingWolvesAudioProcessorEditor::paint(juce::Graphics &g) {
  // 1. Background (Cave)
  if (backgroundImage.isValid())
    g.drawImage(backgroundImage, getLocalBounds().toFloat());
  else
    g.fillAll(juce::Colour::fromString("FF101012")); // Fallback

  auto area = getLocalBounds().toFloat();

  // 1. Remove Top Bar and Keyboard FIRST to define safe Main Area
  auto topBarArea = area.removeFromTop(50.0f);
  auto keyboardArea = area.removeFromBottom(50.0f);

  // Now area is strictly the middle space
  // Now area is strictly the middle space
  auto mainArea = area;
  mainArea.reduce(25, 25); // Increased outer padding from 15 to 25

  // 2. Top Bar Background (Dark Strap - More transparent)
  g.setColour(juce::Colour::fromString("FF0A0A0C").withAlpha(0.7f));
  g.fillRect(topBarArea);
  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRect(topBarArea.removeFromBottom(1), 1.0f); // Bottom separator

  // Title & Subtitle (Left/Center)
  auto titleArea = topBarArea.removeFromLeft(250); // Increased width for logo

  // Use new Vector Logo
  deepCaveLookAndFeel.drawLogo(g, titleArea.removeFromTop(30).toFloat());

  g.setColour(juce::Colours::white.withAlpha(0.6f));
  g.setFont(11.0f); // Reduced font size
  g.drawText("Unleash Your Sound", titleArea, juce::Justification::centredTop,
             false);

  // Preset LCD (Centered between Title and Buttons)
  // topBarArea currently starts after the Title (250px)
  // We need to exclude the button area on the right (220px)
  auto centralArea = topBarArea;
  centralArea.removeFromRight(220);

  auto lcdArea = centralArea.withSizeKeepingCentre(180, 30);

  g.setColour(juce::Colour::fromString("FF000000").withAlpha(0.5f));
  g.fillRoundedRectangle(lcdArea, 4.0f);
  g.setColour(juce::Colour::fromString("FF333333"));
  g.drawRoundedRectangle(lcdArea, 4.0f, 1.0f);
  g.setColour(juce::Colours::white);
  g.setFont(13.0f);

  juce::String currentPreset =
      audioProcessor.getPresetManager().getCurrentPreset();
  if (currentPreset.isEmpty())
    currentPreset = "No Preset Selected";

  g.drawText("PRESET: " + currentPreset, lcdArea.reduced(8),
             juce::Justification::centredLeft, false);

  // 3. Panels (Floating) - strictly within mainArea
  // 3. Panels (Floating) - strictly within mainArea
  mainArea.reduce(10, 10); // Additional inner padding

  float gap = 20.0f; // Increased gap from 15 to 20
  float panelW = (mainArea.getWidth() - gap) / 2.0f;
  float topRowH = mainArea.getHeight() * 0.45f; // Adjusted proportion
  float bottomRowH = mainArea.getHeight() - topRowH - gap;

  // Row 1
  deepCaveLookAndFeel.drawPanel(
      g, juce::Rectangle<float>(mainArea.getX(), mainArea.getY(), 140, topRowH),
      "SOUND ENGINE"); // Left small
  deepCaveLookAndFeel.drawPanel(
      g,
      juce::Rectangle<float>(mainArea.getX() + 140 + gap, mainArea.getY(),
                             mainArea.getWidth() - (140 + gap), topRowH),
      "MODULATION"); // Wide Mod panel

  // Row 2
  float r2y = mainArea.getY() + topRowH + gap;
  deepCaveLookAndFeel.drawPanel(
      g, juce::Rectangle<float>(mainArea.getX(), r2y, panelW, bottomRowH),
      "FILTER & DRIVE");
  deepCaveLookAndFeel.drawPanel(
      g,
      juce::Rectangle<float>(mainArea.getX() + panelW + gap, r2y, panelW,
                             bottomRowH),
      "OUTPUT");

  // Shadow above keyboard (drawn relative to bottom of valid area)
  auto shadowY = getLocalBounds().getBottom() - 50;
  g.setGradientFill(juce::ColourGradient(
      juce::Colours::black.withAlpha(0.8f), 0, (float)shadowY,
      juce::Colours::transparentBlack, 0, (float)shadowY - 20, false));
  g.fillRect(0, shadowY - 20, getWidth(), 20);
}

void HowlingWolvesAudioProcessorEditor::resized() {
  auto area = getLocalBounds();

  // 1. Remove Top Bar and Keyboard FIRST to define safe Main Area
  auto topBar = area.removeFromTop(50);
  auto keyboardArea = area.removeFromBottom(50);

  // 2. Position Fixed Components
  // Buttons
  auto buttonArea = topBar.removeFromRight(220).reduced(5);
  browseButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));
  saveButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));
  settingsButton.setBounds(buttonArea.removeFromLeft(70).reduced(2));

  // Keyboard
  keyboardComponent.setBounds(keyboardArea);

  // 3. Main Body - Layout Panels within the remaining 'area'
  auto mainArea = area;
  mainArea.reduce(25, 25); // Match paint() outer padding

  float gap = 20.0f; // Match paint() gap
  float topRowH = mainArea.getHeight() * 0.45f;

  // Mod Panel starts after Sound Panel (140px) + gap
  auto modPanel = mainArea.withX(mainArea.getX() + 140 + gap)
                      .withWidth(mainArea.getWidth() - (140 + gap))
                      .withHeight(topRowH)
                      .reduced(10, 10); // padding inside panel

  // Place ADSR in Mod Panel (Distributed Evenly)
  // Dynamic Scaling: Calculate Max knob size fitting in the remaining vertical
  // space Available Height = Panel Height - Top Offset (75) - Bottom Margin
  // (10)
  int availableHeight = (int)modPanel.getHeight() - 85;
  int knobSize =
      juce::jlimit(30, 45, availableHeight); // Reduced Max Size from 55 to 45

  juce::FlexBox flexBox;
  flexBox.flexDirection = juce::FlexBox::Direction::row;
  flexBox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
  flexBox.alignContent = juce::FlexBox::AlignContent::center;

  flexBox.items.add(
      juce::FlexItem(attackSlider).withWidth(knobSize).withHeight(knobSize));
  flexBox.items.add(
      juce::FlexItem(decaySlider).withWidth(knobSize).withHeight(knobSize));
  flexBox.items.add(
      juce::FlexItem(sustainSlider).withWidth(knobSize).withHeight(knobSize));
  flexBox.items.add(
      juce::FlexItem(releaseSlider).withWidth(knobSize).withHeight(knobSize));

  // modPanel is float, convert for FlexBox and Account for Title Header (75px)
  auto modContent = modPanel.toNearestInt();
  modContent.removeFromTop(75);
  flexBox.performLayout(modContent);

  // Place Gain in "OUTPUT" Panel (Bottom Right)
  // ... (Gain layout omitted for brevity, assuming it's handled or implicit)

  // (Old block removed)
  // Resume layout for Output Panel
  float bottomRowH =
      mainArea.getHeight() - topRowH - gap; // mainArea is available
  // Re-calculate panelW if needed, or reuse if defined earlier.
  // Wait, panelW was defined in Step 1035 output line 450?
  // No, line 450 defines gap and topRowH. panelW is used in line 500 but not
  // defined in visible block 426-486. Actually panelW IS defined in step 927
  // line 388/397 (in paint) but in resized? Let me define it to be safe.
  // Stack Visualizer (Top) and Gain (Bottom) in Output Panel
  // Re-define panelW if needed (safe scope)
  float panelW = (mainArea.getWidth() - gap) / 2.0f;

  auto outputPanel = mainArea.withY(mainArea.getY() + topRowH + gap)
                         .withX(mainArea.getX() + panelW + gap) // Right side
                         .withWidth(panelW)
                         .withHeight(bottomRowH)
                         .reduced(10);

  auto visualizerSlot =
      outputPanel.removeFromTop(outputPanel.getHeight() * 0.5f);
  // Make flexible but smaller and centered
  // User request: Move down (Trim Top) and Left (Trim Right to push left? Or
  // Trim Left is Shift Right?) "Move to the left" -> Position X decreases. So
  // inside the slot, we want to add right margin (pushing it left) or shift X.

  // Let's apply extra padding:
  // Down: withTrimmedTop(10)
  // Left: withTrimmedRight(10) (Pushes it away from right edge)
  visualizer.setBounds(
      visualizerSlot.reduced(10, 10).withTrimmedTop(15).withTrimmedRight(15));

  // Center Gain in remaining lower half
  gainSlider.setBounds(outputPanel.getCentreX() - knobSize / 2,
                       outputPanel.getCentreY() - knobSize / 2, knobSize,
                       knobSize);

  // Overlay: Preset Browser (Full Area minus Top Bar)
  auto browserArea = getLocalBounds();
  browserArea.removeFromTop(50);
  presetBrowser.setBounds(browserArea);
}
