#include "MidiCapturer.h"

MidiCapturer::MidiCapturer() {}

MidiCapturer::~MidiCapturer() {}

void MidiCapturer::prepare(double sr) { sampleRate = sr; }

void MidiCapturer::processMidi(const juce::MidiBuffer &buffer, int numSamples) {
  if (!recording.load())
    return;

  // We are on Audio Thread! Use lock.
  const juce::ScopedLock sl(lock);

  for (const auto metadata : buffer) {
    auto message = metadata.getMessage();

    // We typically only Capture NoteOn/NoteOff for drag and drop
    // but CC is fine too.

    // Calculate relative time from start of recording
    // Since processMidi is called continuously, we need a running clock?
    // Actually, simple way: we append messages with their sample timestamp
    // relative to the BLOCK. BUT MidiMessageSequence needs absolute time
    // (seconds or ticks).

    // Wait. "The Audio Programmer" pattern:
    // We just keep adding 'message.getTimeStamp()' which is sample offset in
    // buffer? No, we need to accumulate time.

    message.setTimeStamp(startTime + (metadata.samplePosition / sampleRate));
    midiSequence.addEvent(message);
  }

  // Advance global time
  startTime += numSamples / sampleRate;
}

void MidiCapturer::startRecording() {
  const juce::ScopedLock sl(lock);
  midiSequence.clear();
  startTime = 0.0;
  recording.store(true);
}

void MidiCapturer::stopRecording() {
  recording.store(false);
  // Post-process: Ensure NoteOffs?
  // juce::MidiMessageSequence usually handles sorting.

  midiSequence.updateMatchedPairs();

  // We should create the file immediately or wait for UI request?
  // Let's just hold the sequence.
}

bool MidiCapturer::isRecording() const { return recording.load(); }

bool MidiCapturer::hasRecording() const {
  return midiSequence.getNumEvents() > 0;
}

juce::File MidiCapturer::saveToTempFile() {
  const juce::ScopedLock sl(lock);
  if (midiSequence.getNumEvents() == 0)
    return {};

  // Create a temp file
  auto file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                  .getChildFile("WolfMidiCapture.mid");

  if (file.existsAsFile())
    file.deleteFile();

  juce::MidiFile midiFile;
  midiFile.setTicksPerQuarterNote(960); // Standard resolution

  // Convert seconds to ticks?
  // JUCE MidiFile can write tracks based on Seconds if we tell it?
  // Actually, addTrack takes a MidiMessageSequence.
  // By default it expects timestamp in Seconds? "The timestamp of the messages
  // in the sequence are treated as being in seconds" if we don't specify
  // otherwise? Checking docs: "If the MidiFile's time format is set to
  // timestamps in seconds ... otherwise ticks" It defaults to ticks.

  // We recorded in Seconds. We need to convert or set format.
  // MidiFile doesn't support "Seconds" mode for writing standard MIDI files
  // universally compatible. Standard MIDI files use Ticks. So we assume a tempo
  // (e.g. 120 BPM) and convert seconds to ticks.

  juce::MidiMessageSequence ticksSequence;
  double bpm = 120.0; // Assume 120 for drag-drop (most DAWs auto-stretch or
                      // just place it)
  double ticksPerSecond = (960.0 * bpm) / 60.0;

  for (int i = 0; i < midiSequence.getNumEvents(); ++i) {
    auto msg = midiSequence.getEventPointer(i)->message;
    double seconds = msg.getTimeStamp();
    double ticks = seconds * ticksPerSecond;
    msg.setTimeStamp(ticks);
    ticksSequence.addEvent(msg);
  }
  ticksSequence.updateMatchedPairs();

  midiFile.addTrack(ticksSequence);

  if (auto outStream = file.createOutputStream()) {
    midiFile.writeTo(*outStream);
    outStream->flush();
  }

  return file;
}
