#include "SampleManager.h"

SampleManager::SampleManager(SynthEngine &s) : synthEngine(s) {
  formatManager.registerBasicFormats();
}

SampleManager::~SampleManager() {}

void SampleManager::loadSamples() {
  // Initial load can be left empty or load a default welcome sound.
  // We rely on the user selecting a preset.
  synthEngine.clearSounds();
}

void SampleManager::loadSound(const juce::File &file) {
  if (!file.existsAsFile())
    return;

  // Clear current sounds first so we don't play the old one if this load fails
  synthEngine.clearSounds();

  std::unique_ptr<juce::AudioFormatReader> reader(
      formatManager.createReaderFor(file));

  if (reader != nullptr) {
    juce::BigInteger allNotes;
    allNotes.setRange(0, 128, true);

    // Try to read root note from metadata (smpl chunk)
    int rootNote = 60; // C3 default
    if (reader->metadataValues.containsKey("RootNote")) {
      rootNote = reader->metadataValues["RootNote"].getIntValue();
    }

    auto *sound = new HowlingSound(file.getFileNameWithoutExtension(), *reader,
                                   allNotes, rootNote, 0.0, 100.0, 60.0);

    synthEngine.addSound(sound);
  } else {
    DBG("Failed to load sample: " + file.getFullPathName());
  }
}
