#pragma once

#include <JuceHeader.h>

class PresetManager {
public:
  static const juce::File defaultDirectory;
  static const juce::String presetExtension;

  PresetManager(juce::AudioProcessorValueTreeState &);

  void savePreset(const juce::String &presetName,
                  const juce::String &category = "All");
  void deletePreset(const juce::String &presetName);
  void loadPreset(const juce::String &presetName);
  int loadNextPreset();
  int loadPreviousPreset();

  juce::StringArray getAllPresets() const;
  juce::String getCurrentPreset() const;

  juce::File getPresetFolder() const;
  juce::File getPresetFile(const juce::String &presetName) const;

private:
  void valueTreeRedirected(juce::ValueTree &treeThatChanged);

  juce::AudioProcessorValueTreeState &valueTreeState;
  juce::String currentPresetName;
};
