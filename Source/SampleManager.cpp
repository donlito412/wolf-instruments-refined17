#include "SampleManager.h"

SampleManager::SampleManager(SynthEngine &s) : synthEngine(s) {
  formatManager.registerBasicFormats();
}

SampleManager::~SampleManager() {}

void SampleManager::loadSamples() {
  synthEngine.clearSounds();
  synthEngine.initialize(); // Ensure voices are added

  // 1. Create a "Melody" sound (Triangle wave)
  // Mapped from C3 (60) to C5 (84)
  // Root C4 (72)
  loadPlaceholderSound("Melody", 72, juce::Range<int>(60, 85),
                       true); // Loop enabled

  // 2. Create a "Kick Drum" sound (Sine/Noise burst)
  // Mapped to C1 (36) only
  loadPlaceholderSound("Kick", 36, juce::Range<int>(36, 37), false); // One shot
}

void SampleManager::loadPlaceholderSound(const juce::String &name, int rootNote,
                                         juce::Range<int> noteRange,
                                         bool loop) {
  // Create a 2-second buffer at 44.1kHz
  double sampleRate = 44100.0;
  int lengthSamples = (int)(sampleRate * 2.0);
  juce::AudioBuffer<float> buffer(1, lengthSamples);

  auto *w = buffer.getWritePointer(0);
  double phase = 0.0;
  double freq = 440.0; // A4

  // Adjust freq based on rootNote to match roughly standard tuning if we
  // wanted, but for placeholder, let's just make a tone. Actually, if we map it
  // to 'rootNote', and the synth pitch shifts, we should generate 'rootNote'
  // frequency.
  freq = juce::MidiMessage::getMidiNoteInHertz(rootNote);
  double inc = (freq * 2.0 * juce::MathConstants<double>::pi) / sampleRate;

  for (int i = 0; i < lengthSamples; ++i) {
    // Simple decay envelope
    float env = 1.0f - (float)i / lengthSamples;

    // Triangle-ish wave
    float samp = (float)(std::sin(phase) + 0.5 * std::sin(phase * 2.0));
    w[i] = samp * env * 0.5f;

    phase += inc;
  }

  juce::BigInteger notes;
  notes.setRange(noteRange.getStart(), noteRange.getLength(), true);

  // Attack 0.01s, Release 0.1s, MaxLen 10s
  auto *sound = new HowlingSound(name, buffer, sampleRate, notes, rootNote,
                                 0.01, 0.2, 10.0);
  sound->setLooping(loop);

  synthEngine.addSound(sound);
}
