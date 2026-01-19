#pragma once

#include "SynthEngine.h"
#include <JuceHeader.h>

//==============================================================================
/**
    Manages loading of samples and mapping them to the synth.
*/
class SampleManager {
public:
  SampleManager(SynthEngine &synth);
  ~SampleManager();

  void loadSamples();

private:
  SynthEngine &synthEngine;
  juce::AudioFormatManager formatManager;

  // Helper to load from BinaryData (commented out until assets exist)
  // void loadSound(const char* data, int dataSize, const juce::String& name,
  // int note);

  // Helper to generate a placeholder beep
  void loadPlaceholderSound(const juce::String &name, int rootNote,
                            juce::Range<int> noteRange, bool loop);
};
