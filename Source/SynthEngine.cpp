#include "SynthEngine.h"

//==============================================================================
// HowlingSound
//==============================================================================

HowlingSound::HowlingSound(const juce::String &soundName,
                           juce::AudioBuffer<float> &content, double sourceRate,
                           const juce::BigInteger &notes, int rootNote,
                           double attackSecs, double releaseSecs, double maxLen)
    : name(soundName), sourceSampleRate(sourceRate), midiNotes(notes),
      midiNoteForNormalPitch(rootNote) {
  // deep copy the buffer for safety
  data.makeCopyOf(content);

  attack = attackSecs;
  release = releaseSecs;
}

bool HowlingSound::appliesToNote(int midiNoteNumber) {
  return midiNotes[midiNoteNumber];
}

bool HowlingSound::appliesToChannel(int midiChannel) { return true; }

//==============================================================================
// HowlingVoice
//==============================================================================

HowlingVoice::HowlingVoice() {}

bool HowlingVoice::canPlaySound(juce::SynthesiserSound *sound) {
  return dynamic_cast<HowlingSound *>(sound) != nullptr;
}

void HowlingVoice::startNote(int midiNoteNumber, float velocity,
                             juce::SynthesiserSound *sound,
                             int currentPitchWheelPosition) {
  if (auto *howlingSound = dynamic_cast<HowlingSound *>(sound)) {
    sourceSamplePosition = 0.0;
    level = velocity;
    isLooping = howlingSound->isLooping();

    // Calculate pitch ratio
    auto sourceRate = howlingSound->getSourceSampleRate();
    auto rootNote = howlingSound->getMidiNoteForNormalPitch();

    // Pitch ratio = (target_freq / source_freq) * (source_rate / target_rate)
    // Simple formula based on semitones:
    auto semitones = midiNoteNumber - rootNote;
    auto pitchFactor = std::pow(2.0, semitones / 12.0);

    pitchRatio = pitchFactor * (sourceRate / getSampleRate());

    // Setup ADSR
    adsrParams.attack = static_cast<float>(howlingSound->attack);
    adsrParams.decay = 0.0f;
    adsrParams.sustain = 1.0f;
    adsrParams.release = static_cast<float>(howlingSound->release);

    adsr.setSampleRate(getSampleRate());
    adsr.setParameters(adsrParams);
    adsr.noteOn();
  } else {
    jassertfalse; // Should not happen
    clearCurrentNote();
  }
}

void HowlingVoice::stopNote(float velocity, bool allowTailOff) {
  if (allowTailOff) {
    adsr.noteOff();
  } else {
    clearCurrentNote();
    adsr.reset();
  }
}

void HowlingVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   int startSample, int numSamples) {
  if (auto *playingSound =
          dynamic_cast<HowlingSound *>(getCurrentlyPlayingSound().get())) {
    auto &data = playingSound->getAudioData();
    const float *const inL = data.getReadPointer(0);
    const float *const inR =
        data.getNumChannels() > 1 ? data.getReadPointer(1) : nullptr;

    float *outL = outputBuffer.getWritePointer(0, startSample);
    float *outR = outputBuffer.getNumChannels() > 1
                      ? outputBuffer.getWritePointer(1, startSample)
                      : nullptr;

    while (--numSamples >= 0) {
      auto currentPos = (int)sourceSamplePosition;
      auto nextPos = currentPos + 1;
      auto alpha = (float)(sourceSamplePosition - currentPos);
      auto invAlpha = 1.0f - alpha;

      // Linear Interpolation
      // Check bounds
      if (nextPos >= data.getNumSamples()) {
        if (isLooping) {
          nextPos = 0; // simplistic loop
          // If currentPos was exact end, prevent OOB?
          if (currentPos >= data.getNumSamples())
            currentPos = 0;
        } else {
          // Stop playing if end reached
          stopNote(0.0f, false);
          break;
        }
      }

      float l = (inL[currentPos] * invAlpha + inL[nextPos] * alpha);
      float r = (inR != nullptr)
                    ? (inR[currentPos] * invAlpha + inR[nextPos] * alpha)
                    : l;

      auto env = adsr.getNextSample();

      if (outL != nullptr)
        *outL++ += l * level * env;
      if (outR != nullptr)
        *outR++ += r * level * env;

      sourceSamplePosition += pitchRatio;

      // Handle Looping pointer wrap
      if (isLooping) {
        if (sourceSamplePosition >= data.getNumSamples())
          sourceSamplePosition -= data.getNumSamples();
      } else {
        if (sourceSamplePosition >= data.getNumSamples()) {
          stopNote(0.0f, false);
          break;
        }
      }

      if (!adsr.isActive()) {
        clearCurrentNote();
        break;
      }
    }
  }
}

void HowlingVoice::updateADSR(const juce::ADSR::Parameters &params) {
  adsrParams = params;
  adsr.setParameters(adsrParams);
}

//==============================================================================
// SynthEngine
//==============================================================================

SynthEngine::SynthEngine() {}

void SynthEngine::initialize() {
  // Add voices
  // We want 16 voices
  for (int i = 0; i < 16; i++) {
    addVoice(new HowlingVoice());
  }
}

void SynthEngine::updateADSR(float attack, float decay, float sustain,
                             float release) {
  juce::ADSR::Parameters params;
  params.attack = attack;
  params.decay = decay;
  params.sustain = sustain;
  params.release = release;

  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateADSR(params);
    }
  }
}
