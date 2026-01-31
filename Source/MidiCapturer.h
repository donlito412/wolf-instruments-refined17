#pragma once

#include <JuceHeader.h>
#include <atomic>

class MidiCapturer {
public:
  MidiCapturer();
  ~MidiCapturer();

  void prepare(double sampleRate);
  void processMidi(const juce::MidiBuffer &buffer, int numSamples);

  void startRecording();
  void stopRecording();
  bool isRecording() const;

  juce::File getLastRecording() const;
  bool hasRecording() const;

  // Creates the .mid file and returns it
  juce::File saveToTempFile();

private:
  std::atomic<bool> recording{false};
  juce::MidiMessageSequence midiSequence;
  double sampleRate = 44100.0;
  double startTime = 0.0;

  juce::CriticalSection lock; // Protect sequence access
  juce::File lastFile;
};
