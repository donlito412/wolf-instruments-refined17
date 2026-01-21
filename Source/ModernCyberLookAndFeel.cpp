#include "ModernCyberLookAndFeel.h"

ModernCyberLookAndFeel::ModernCyberLookAndFeel() {
  // Set default colors
  setColour(juce::ResizableWindow::backgroundColourId, WolfColors::BG_CAVE);
  setColour(juce::TextButton::buttonColourId, WolfColors::CONTROL_BG);
  setColour(juce::TextButton::textColourOffId, WolfColors::TEXT_PRIMARY);
  setColour(juce::TextButton::textColourOnId, juce::Colours::white);
  setColour(juce::Slider::backgroundColourId, WolfColors::CONTROL_BG);
  setColour(juce::Slider::thumbColourId, WolfColors::TEXT_PRIMARY);
  setColour(juce::Slider::trackColourId, WolfColors::ACCENT_CYAN);
}

void ModernCyberLookAndFeel::drawTabButton(juce::TabBarButton &button,
                                           juce::Graphics &g, bool isMouseOver,
                                           bool isMouseDown) {
  auto area = button.getActiveArea().toFloat();
  auto isActive = button.getToggleState();

  // Background (only for active tab)
  if (isActive) {
    g.setColour(WolfColors::PANEL_DARK);
    g.fillRoundedRectangle(area, 3.0f);
  }

  // Bottom border for active tab (cyan accent)
  if (isActive) {
    g.setColour(WolfColors::ACCENT_CYAN);
    g.fillRect(area.getX(), area.getBottom() - 2.0f, area.getWidth(), 2.0f);
  }

  // Text
  g.setColour(isActive ? WolfColors::ACCENT_CYAN : WolfColors::TEXT_SECONDARY);
  g.setFont(12.0f);
  g.drawText(button.getButtonText(), area, juce::Justification::centred);
}

void ModernCyberLookAndFeel::drawTabbedButtonBarBackground(
    juce::TabbedButtonBar &bar, juce::Graphics &g) {
  // Dark background for tab bar
  g.setColour(WolfColors::PANEL_DARKER);
  g.fillRect(bar.getLocalBounds());

  // Subtle bottom border
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.fillRect(0, bar.getHeight() - 1, bar.getWidth(), 1);
}

int ModernCyberLookAndFeel::getTabButtonBestWidth(juce::TabBarButton &button,
                                                  int tabDepth) {
  return 120; // Fixed width for clean appearance
}

void ModernCyberLookAndFeel::drawButtonBackground(
    juce::Graphics &g, juce::Button &button,
    const juce::Colour &backgroundColour, bool isMouseOverButton,
    bool isButtonDown) {
  auto bounds = button.getLocalBounds().toFloat();
  auto isToggled = button.getToggleState();

  // Background
  g.setColour(isButtonDown        ? WolfColors::ACCENT_TEAL
              : isToggled         ? WolfColors::ACCENT_CYAN
              : isMouseOverButton ? WolfColors::CONTROL_BG.brighter(0.1f)
                                  : backgroundColour);
  g.fillRoundedRectangle(bounds, 3.0f);

  // Subtle border
  g.setColour(WolfColors::BORDER_SUBTLE);
  g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

void ModernCyberLookAndFeel::drawLinearSlider(
    juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
    float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider &slider) {
  if (style == juce::Slider::LinearHorizontal) {
    // Track background
    auto trackY = y + height / 2 - 2;
    g.setColour(WolfColors::CONTROL_BG);
    g.fillRoundedRectangle(x, trackY, width, 4, 2.0f);

    // Cyan fill from start to slider position
    g.setColour(WolfColors::ACCENT_CYAN);
    g.fillRoundedRectangle(x, trackY, sliderPos - x, 4, 2.0f);

    // Thumb (small circle)
    g.setColour(WolfColors::TEXT_PRIMARY);
    g.fillEllipse(sliderPos - 6, y + height / 2 - 6, 12, 12);

    // Subtle thumb border
    g.setColour(WolfColors::ACCENT_CYAN);
    g.drawEllipse(sliderPos - 6, y + height / 2 - 6, 12, 12, 1.0f);
  }
}

void ModernCyberLookAndFeel::drawRotarySlider(
    juce::Graphics &g, int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider &slider) {

  auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
  auto center = bounds.getCentre();
  auto radius = juce::jmin(width, height) / 2.0f - 10.0f; // Padding

  // 1. Tick Marks (Interactive)
  int numTicks = 15;
  // Calculate angle based on slider pos
  float currentAngle =
      rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
  float angleStep = (rotaryEndAngle - rotaryStartAngle) / (numTicks - 1);

  for (int i = 0; i < numTicks; ++i) {
    float angle = rotaryStartAngle + i * angleStep;
    // Active ticks are those passed by the knob
    bool isActive = angle <= currentAngle + 0.01f;

    if (isActive && slider.isEnabled())
      g.setColour(WolfColors::ACCENT_CYAN);
    else
      g.setColour(WolfColors::ACCENT_CYAN.withAlpha(0.2f));

    auto tickStart = center.getPointOnCircumference(radius + 2.0f, angle);
    auto tickEnd = center.getPointOnCircumference(radius + 6.0f, angle);
    g.drawLine(juce::Line<float>(tickStart, tickEnd), 1.5f);
  }

  // 2. Brushed Metal Center
  auto knobRadius = radius - 2.0f;
  juce::ColourGradient metalGradient(
      juce::Colour(0xff444444), center.getX() - knobRadius,
      center.getY() - knobRadius, juce::Colour(0xff222222),
      center.getX() + knobRadius, center.getY() + knobRadius, true);
  metalGradient.addColour(0.5, juce::Colour(0xff333333));
  g.setGradientFill(metalGradient);
  g.fillEllipse(center.getX() - knobRadius, center.getY() - knobRadius,
                knobRadius * 2, knobRadius * 2);

  // Metallic Rim
  g.setColour(juce::Colour(0xff666666));
  g.drawEllipse(center.getX() - knobRadius, center.getY() - knobRadius,
                knobRadius * 2, knobRadius * 2, 2.0f);

  // 3. Progress Arc (Cyan)
  juce::Path arcPath;
  float arcRadius = knobRadius - 3.0f;

  // Background Arc
  juce::Path bgArc;
  bgArc.addCentredArc(center.getX(), center.getY(), arcRadius, arcRadius, 0.0f,
                      rotaryStartAngle, rotaryEndAngle, true);
  g.setColour(juce::Colour(0xff111111));
  g.strokePath(bgArc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                           juce::PathStrokeType::rounded));

  // Active Arc
  arcPath.addCentredArc(center.getX(), center.getY(), arcRadius, arcRadius,
                        0.0f, rotaryStartAngle, currentAngle, true);
  g.setColour(WolfColors::ACCENT_CYAN);
  g.strokePath(arcPath, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
  g.setColour(WolfColors::ACCENT_GLOW);
  g.strokePath(arcPath, juce::PathStrokeType(8.0f, juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));

  // 4. Indicator (Pointer) - DOT
  juce::Path pointerPath;
  float dotSize = 5.0f;
  // Position dot at the rim, rotating with value
  float dotY = -knobRadius + 8.0f;
  pointerPath.addEllipse(-dotSize / 2.0f, dotY - dotSize / 2.0f, dotSize,
                         dotSize);

  pointerPath.applyTransform(
      juce::AffineTransform::rotation(currentAngle).translated(center));
  g.setColour(WolfColors::TEXT_PRIMARY);
  g.fillPath(pointerPath);
}
