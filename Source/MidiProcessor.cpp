#include "MidiProcessor.h"

//==============================================================================
// Arpeggiator
//==============================================================================

Arpeggiator::Arpeggiator() {}

void Arpeggiator::prepare(double sampleRate) { currentSampleRate = sampleRate; }

void Arpeggiator::reset() {
  sortedNotes.clear();
  currentStep = 0;
  noteTime = 0.0;
  // Panic: Clear active notes to stop stuck ones on reset
  activeNotes.clear();
}

void Arpeggiator::setParameters(float rate, int mode, int octaves, float gate,
                                bool on, float density, float complexity,
                                float spread) {
  rateDiv = rate;
  arpMode = mode;
  numOctaves = octaves;
  gateLength = gate;
  enabled = on;
  arpDensity = density;       // 0.0-1.0
  arpComplexity = complexity; // 0.0-1.0
  arpSpread = spread;
}

void Arpeggiator::setRhythmStep(int step, bool active) {
  if (step >= 0 && step < 16)
    rhythmPattern[(size_t)step] = active;
}

bool Arpeggiator::getRhythmStep(int step) const {
  if (step >= 0 && step < 16)
    return rhythmPattern[(size_t)step];
  return false;
}

void Arpeggiator::handleNoteOn(int note, int velocity) {
  bool wasEmpty = sortedNotes.empty();

  // Add unique and sort
  bool found = false;
  for (int n : sortedNotes)
    if (n == note)
      found = true;

  if (!found) {
    sortedNotes.push_back(note);
    std::sort(sortedNotes.begin(), sortedNotes.end());
  }

  // Instant Trigger: If this is the first note, force trigger ASAP
  if (wasEmpty) {
    currentStep = 0;
    noteTime = 1000000.0;
  }
}

void Arpeggiator::handleNoteOff(int note) {
  // Remove
  auto it = std::remove(sortedNotes.begin(), sortedNotes.end(), note);
  sortedNotes.erase(it, sortedNotes.end());
}

int Arpeggiator::getNextNote() {
  if (sortedNotes.empty())
    return -1;

  int numNotes = (int)sortedNotes.size();
  int totalSteps = numNotes * numOctaves;

  if (totalSteps == 0)
    return -1;

  int effectiveStep = 0;

  // Mode Logic based on arpMode parameter
  switch (arpMode) {
  case 0: // Up
    effectiveStep = currentStep % totalSteps;
    break;
  case 1: // Down
    effectiveStep = (totalSteps - 1) - (currentStep % totalSteps);
    break;
  case 2: // Up/Down (Ping Pong)
  {
    if (totalSteps <= 1) {
      effectiveStep = 0;
    } else {
      int sequenceLen = (totalSteps * 2) - 2;
      int phase = currentStep % sequenceLen;
      effectiveStep = (phase < totalSteps) ? phase : (sequenceLen - phase);
    }
  } break;
  case 3: // Random
    effectiveStep = juce::Random::getSystemRandom().nextInt(totalSteps);
    break;
  default:
    effectiveStep = currentStep % totalSteps;
    break;
  }

  int noteIndex = effectiveStep % numNotes;
  int octaveOffset = effectiveStep / numNotes;

  int note = sortedNotes[(size_t)noteIndex] + (octaveOffset * 12);

  // Range check
  if (note > 127)
    note = 127;

  return note;
}

double Arpeggiator::getSamplesPerStep(juce::AudioPlayHead *playHead) {
  double bpm = 120.0;
  if (playHead) {
    if (auto pos = playHead->getPosition()) {
      if (pos->getBpm().hasValue())
        bpm = *pos->getBpm();
    }
  }

  // Safety Clamp
  if (bpm < 20.0)
    bpm = 120.0;

  // RateDiv: 0=1/4, 1=1/8...
  double quarterNoteSamples = (60.0 / bpm) * currentSampleRate;

  if (rateDiv <= 0.1f)
    return quarterNoteSamples; // 1/4
  if (rateDiv <= 0.4f)
    return quarterNoteSamples / 2.0; // 1/8
  if (rateDiv <= 0.7f)
    return quarterNoteSamples / 4.0; // 1/16
  return quarterNoteSamples / 8.0;   // 1/32
}

void Arpeggiator::process(juce::MidiBuffer &midiMessages, int numSamples,
                          juce::AudioPlayHead *playHead) {

  // --- 1. ALWAYS Update Internal State (Input Sensing) ---
  // We scan the buffer to keep sortedNotes up to date,
  // regardless of whether Arp is enabled or not.
  // This prevents "Stale State" where released keys stick in memory.

  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();
    if (msg.isNoteOn()) {
      handleNoteOn(msg.getNoteNumber(), msg.getVelocity());
    } else if (msg.isNoteOff()) {
      handleNoteOff(msg.getNoteNumber());
    } else if (msg.isAllNotesOff()) {
      reset();
    }
  }

  // --- 2. If DISABLED: Flush active notes and Return (Pass-Through) ---
  if (!enabled) {
    // Just ensure any lingering arp notes are killed
    for (auto it = activeNotes.begin(); it != activeNotes.end();) {
      int noteOffPos = it->samplesRemaining;
      if (noteOffPos < numSamples) {
        midiMessages.addEvent(juce::MidiMessage::noteOff(1, it->noteNumber),
                              noteOffPos);
        it = activeNotes.erase(it);
      } else {
        it->samplesRemaining -= numSamples;
        ++it;
      }
    }
    // midiMessages remains untouched (contains original input notes)
    return;
  }

  // --- 3. If ENABLED: Filter Input & Generate Arp ---

  // Create a clean buffer. We will populate it with everything EXCEPT input
  // notes.
  juce::MidiBuffer processedMidi;

  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();
    if (!msg.isNoteOn() && !msg.isNoteOff()) {
      // Pass thru non-note events (CC, PitchBend)
      processedMidi.addEvent(msg, metadata.samplePosition);
    }
  }

  // Append Active Notes (Pending Offs)
  for (auto it = activeNotes.begin(); it != activeNotes.end();) {
    int noteOffPos = it->samplesRemaining;
    if (noteOffPos < numSamples) {
      processedMidi.addEvent(juce::MidiMessage::noteOff(1, it->noteNumber),
                             noteOffPos);
      it = activeNotes.erase(it);
    } else {
      it->samplesRemaining -= numSamples;
      ++it;
    }
  }

  // Generate Arp Notes
  if (sortedNotes.empty()) {
    midiMessages.swapWith(processedMidi); // Output clean buffer (silence)
    return;
  }

  double samplesPerStep = getSamplesPerStep(playHead);
  if (samplesPerStep < 100.0)
    samplesPerStep = 10000.0;

  if (noteTime > samplesPerStep)
    noteTime = samplesPerStep;

  int samplesRemaining = numSamples;
  int currentSamplePos = 0;

  while (samplesRemaining > 0) {
    if (noteTime >= samplesPerStep) {
      noteTime -= samplesPerStep;

      int noteToPlay = getNextNote();

      if (noteToPlay > 0) {
        // Density Check: simple probability
        // If density < 1.0, we might skip this step.
        // density 1.0 = play always (except if random gave -1 which shouldn't
        // happen here) density 0.0 = never play

        bool shouldPlay = true;

        // 1. Check Rhythm Pattern (Grid)
        int stepIdx = currentStep % 16;
        if (!rhythmPattern[(size_t)stepIdx]) {
          shouldPlay = false;
        }

        // 2. Check Density (Probability)
        if (shouldPlay && arpDensity < 0.99f) {
          float rnd = juce::Random::getSystemRandom().nextFloat();
          if (rnd > arpDensity)
            shouldPlay = false;
        }

        if (shouldPlay) {
          // Complexity: Chance to jump octave or add variety
          int finalNote = noteToPlay;
          if (arpComplexity > 0.01f) {
            float rnd = juce::Random::getSystemRandom().nextFloat();
            if (rnd < arpComplexity) {
              // 50% chance to go up, 50% down? Or just up?
              // Let's go UP an octave for "complex" flair
              finalNote += 12;
              if (finalNote > 127)
                finalNote -= 24;
            }
          }

          // Add Note On to output
          processedMidi.addEvent(
              juce::MidiMessage::noteOn(1, finalNote, (juce::uint8)100),
              currentSamplePos);

          int gateSamples = static_cast<int>(samplesPerStep * gateLength);

          if (currentSamplePos + gateSamples < numSamples) {
            // Note Off fits in this block
            processedMidi.addEvent(juce::MidiMessage::noteOff(1, finalNote),
                                   currentSamplePos + gateSamples);
          } else {
            // Note Off is in future block
            ActiveNote an;
            an.noteNumber = finalNote;
            an.samplesRemaining = gateSamples - (numSamples - currentSamplePos);
            activeNotes.push_back(an);
          }
        }
      }
      currentStep++;
    }

    int processAmount = std::min(samplesRemaining, 32);
    noteTime += processAmount;
    samplesRemaining -= processAmount;
    currentSamplePos += processAmount;
  }

  // Final Swap
  midiMessages.swapWith(processedMidi);
}

//==============================================================================
// ChordEngine
//==============================================================================

ChordEngine::ChordEngine() {}

void ChordEngine::setParameters(int mode, int keys, bool hold) {
  chordMode = mode;
  bool wasHolding = holdEnabled;
  holdEnabled = hold;

  // If we turned OFF hold, kill all stuck notes
  if (wasHolding && !holdEnabled) {
    shouldFlushNotes = true;
  }
}

void ChordEngine::process(juce::MidiBuffer &midiMessages) {
  juce::MidiBuffer processedBuf;

  // 1. Flush Stuck Notes if Hold was just disabled
  if (shouldFlushNotes) {
    for (int note : heldNotes) {
      processedBuf.addEvent(juce::MidiMessage::noteOff(1, note), 0);
    }
    heldNotes.clear();
    shouldFlushNotes = false;
  }

  // Interpret "Chord Hold" button as "Enable Chords" (User Request)
  // If button is ON (holdEnabled) but Mode is OFF, force Mode to MINOR (2)
  int effectiveMode = chordMode;
  if (holdEnabled && effectiveMode == 0) {
    effectiveMode = 2; // Default to Minor
  }

  if (effectiveMode == 0) {
    return;
  }

  // Clear original output buffer (we rebuild it)
  // Actually, standard practice for these processors: read from 'midiMessages',
  // write to 'processedBuf', then swap. BUT if we 'return', 'midiMessages'
  // keeps original content (Passthrough). So for Mode=0 returns, it's
  // Passthrough.

  for (const auto metadata : midiMessages) {
    auto msg = metadata.getMessage();

    if (msg.isNoteOn() || msg.isNoteOff()) {
      int root = msg.getNoteNumber();
      int vel = msg.getVelocity();
      bool isOn = msg.isNoteOn();

      // HOLD LOGIC:
      // If NoteOff AND Hold is ON -> Ignore (don't add to processedBuf)
      // But we must remember it to kill it later.
      if (!isOn && holdEnabled) {
        // Ignore Note Off
        continue;
      }

      auto addEvent = [&](int note) {
        if (isOn) {
          processedBuf.addEvent(
              juce::MidiMessage::noteOn(1, note, (juce::uint8)vel),
              metadata.samplePosition);
          if (holdEnabled)
            heldNotes.insert(note);
        } else {
          processedBuf.addEvent(juce::MidiMessage::noteOff(1, note),
                                metadata.samplePosition);
          if (heldNotes.find(note) != heldNotes.end()) {
            heldNotes.erase(note);
          }
        }
      };

      addEvent(root);

      switch (effectiveMode) {
      case 1: // Major
        addEvent(root + 4);
        addEvent(root + 7);
        break;
      case 2: // Minor
        addEvent(root + 3);
        addEvent(root + 7);
        break;
      case 3: // 7th
        addEvent(root + 4);
        addEvent(root + 7);
        addEvent(root + 10);
        break;
      case 4: // 9th
        addEvent(root + 4);
        addEvent(root + 7);
        addEvent(root + 14);
        break;
      default:
        break;
      }

    } else {
      processedBuf.addEvent(msg, metadata.samplePosition);
    }
  }

  midiMessages.swapWith(processedBuf);
}

//==============================================================================
// MidiProcessor
//==============================================================================

MidiProcessor::MidiProcessor() {}

void MidiProcessor::prepare(double sampleRate) {
  currentSampleRate = sampleRate;
  arp.prepare(sampleRate);
}

void MidiProcessor::reset() { arp.reset(); }

void MidiProcessor::process(juce::MidiBuffer &midiMessages, int numSamples,
                            juce::AudioPlayHead *playHead) {
  // 1. Chords First
  chordEngine.process(midiMessages);

  // 2. Arp Second
  arp.process(midiMessages, numSamples, playHead);
}
