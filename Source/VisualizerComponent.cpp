#include "VisualizerComponent.h"

VisualizerComponent::VisualizerComponent() {
  startTimerHz(30); // 30 FPS Refresh
  scopeData.fill(0.0f);
}

VisualizerComponent::~VisualizerComponent() { stopTimer(); }

void VisualizerComponent::pushBuffer(const juce::AudioBuffer<float> &buffer) {
  if (buffer.getNumChannels() > 0) {
    auto *channelData = buffer.getReadPointer(0);
    int numSamples = buffer.getNumSamples();

    int start1, size1, start2, size2;
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
      std::copy(channelData, channelData + size1, fifoBuffer.data() + start1);
    if (size2 > 0)
      std::copy(channelData + size1, channelData + size1 + size2,
                fifoBuffer.data() + start2);

    fifo.finishedWrite(size1 + size2);
  }
}

void VisualizerComponent::timerCallback() {
  // Read from FIFO to fill scopeData
  int numSamples = fifo.getNumReady();
  if (numSamples > 0) {
    std::array<float, 4096> tempBuffer;
    int start1, size1, start2, size2;
    fifo.prepareToRead(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
      std::copy(fifoBuffer.data() + start1, fifoBuffer.data() + start1 + size1,
                tempBuffer.data());
    if (size2 > 0)
      std::copy(fifoBuffer.data() + start2, fifoBuffer.data() + start2 + size2,
                tempBuffer.data() + size1);

    fifo.finishedRead(size1 + size2);

    // Update scopeData (Simple rolling or trigger? Let's just grab the latest
    // chunk) Actually, for a visualizer, we want to shift data or just replace
    // it. Let's just copy exactly 'scopeSize' from the END of what we read, or
    // shift.

    for (int i = 0; i < size1 + size2; ++i) {
      // Shift left
      for (int j = 0; j < scopeSize - 1; ++j)
        scopeData[j] = scopeData[j + 1];
      scopeData[scopeSize - 1] = tempBuffer[i];
    }
    repaint();
  }
}

void VisualizerComponent::paint(juce::Graphics &g) {
  auto area = getLocalBounds();
  g.setColour(juce::Colour::fromString("FF111111")
                  .withAlpha(0.5f)); // Semi-transparent bg
  g.fillRoundedRectangle(area.toFloat(), 5.0f);

  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRoundedRectangle(area.toFloat(), 5.0f, 1.0f); // Border

  g.setColour(juce::Colour::fromString("FF88CCFF")); // Ice Blue Wave

  juce::Path wavePath;
  float w = (float)getWidth();
  float h = (float)getHeight();
  float centerY = h * 0.5f;
  float scaleY = h * 0.45f; // Almost full height

  wavePath.startNewSubPath(0, centerY + scopeData[0] * scaleY);
  float xInc = w / (float)scopeSize;

  for (int i = 1; i < scopeSize; ++i) {
    wavePath.lineTo((float)i * xInc, centerY + scopeData[i] * scaleY);
  }

  g.strokePath(wavePath, juce::PathStrokeType(1.5f));
}
