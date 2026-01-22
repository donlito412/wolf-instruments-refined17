#include "SynthEngine.h"

//==============================================================================
// HowlingVoice
//==============================================================================

HowlingVoice::HowlingVoice() {
  // Placeholder init
  lfo.initialise([](float x) { return std::sin(x); });

  // Initialize ADSR with default
  adsr.setSampleRate(44100.0); // Will be updated in prepare
  adsrParams = {0.1f, 0.1f, 1.0f, 0.1f};
  adsr.setParameters(adsrParams);
}

void HowlingVoice::prepare(double sampleRate, int samplesPerBlock) {
  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = samplesPerBlock;
  spec.numChannels = 1; // Mono voice

  filter.prepare(spec);
  filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

  lfo.prepare(spec);

  adsr.setSampleRate(sampleRate);

  // Resize temp buffer for processing
  tempBuffer.setSize(1, samplesPerBlock); // Mono voice
}

void HowlingVoice::updateFilter(float cutoff, float resonance) {
  baseCutoff = cutoff;
  baseResonance = resonance;
  filter.setCutoffFrequency(cutoff);
  filter.setResonance(resonance);
}

void HowlingVoice::updateLFO(float rate, float depth) {
  lfo.setFrequency(rate);
  lfoDepth = depth;
}

void HowlingVoice::updateADSR(float attack, float decay, float sustain,
                              float release) {
  adsrParams.attack = attack;
  adsrParams.decay = decay;
  adsrParams.sustain = sustain;
  adsrParams.release = release;
  adsr.setParameters(adsrParams);
}

void HowlingVoice::updateSampleParams(float tune, float sampleStart,
                                      float sampleEnd, bool loop) {
  tuneSemitones = tune;
  sampleStartPercent = sampleStart;
  sampleEndPercent = sampleEnd;
  isLooping = loop;
}

void HowlingVoice::startNote(int midiNoteNumber, float velocity,
                             juce::SynthesiserSound *sound,
                             int currentPitchWheelPosition) {
  // 1. Base startNote
  juce::SamplerVoice::startNote(midiNoteNumber, velocity, sound,
                                currentPitchWheelPosition);

  // 2. Tune (Pitch Shift)
  // DISABLED: Requires private pitchRatio access
  /*
  if (tuneSemitones != 0.0f) {
    // Pitch ratio modification
    double tuneRatio = std::pow(2.0, tuneSemitones / 12.0);
    pitchRatio *= tuneRatio;
  }
  */

  // 3. Sample Start Offset
  // DISABLED: Requires private sourceSamplePosition access
  /*
  if (auto *samplerSound = dynamic_cast<juce::SamplerSound *>(sound)) {
    if (auto *audioData = samplerSound->getAudioData()) {
      int64 length = audioData->getNumSamples();
      int64 startPos = static_cast<int64>(length * sampleStartPercent);

      // sourceSamplePosition is protected in Juce::SamplerVoice
      sourceSamplePosition = static_cast<double>(startPos);
    }
  }
  */

  adsr.noteOn();
  filter.reset();
  lfo.reset();
}

void HowlingVoice::stopNote(float velocity, bool allowTailOff) {
  if (allowTailOff) {
    adsr.noteOff();
    juce::SamplerVoice::stopNote(velocity, true);
  } else {
    adsr.reset();
    juce::SamplerVoice::stopNote(velocity, false);
  }
}

void HowlingVoice::renderNextBlock(juce::AudioBuffer<float> &outputBuffer,
                                   int startSample, int numSamples) {
  if (!isVoiceActive())
    return;

  if (tempBuffer.getNumSamples() < numSamples) {
    tempBuffer.setSize(1, numSamples, false, false, true);
  }
  tempBuffer.clear();

  // 1. Check Sample End Truncation
  // DISABLED: Requires private sourceSamplePosition access
  /*
  if (auto *sound = dynamic_cast<juce::SamplerSound *>(
          getCurrentlyPlayingSound().get())) {
    if (auto *audioData = sound->getAudioData()) {
      int64 length = audioData->getNumSamples();
      int64 endPos = static_cast<int64>(length * sampleEndPercent);

      // If we passed the end point (and valid end point), stop.
      // Ignore if looping (standard sampler handles loop points usually defined
      // in file, but we can try to force stop if we want custom region loop -
      // Phase 2). For now, if not looping, stop at End slider.
      if (sourceSamplePosition >= endPos && !isLooping) {
        stopNote(0.0f, false);
        return;
      }
    }
  }
  */

  // 2. Render
  juce::SamplerVoice::renderNextBlock(tempBuffer, 0, numSamples);

  // 3. ADSR
  adsr.applyEnvelopeToBuffer(tempBuffer, 0, numSamples);

  auto *bufferData = tempBuffer.getWritePointer(0);

  for (int i = 0; i < numSamples; ++i) {
    float lfoValue = lfo.processSample(0.0f);

    float modFactor = std::pow(2.0f, lfoValue * lfoDepth * 2.0f);
    float modCutoff = baseCutoff * modFactor;
    modCutoff = juce::jlimit(20.0f, 20000.0f, modCutoff);

    filter.setCutoffFrequency(modCutoff);
    filter.setResonance(baseResonance);

    float input = bufferData[i];
    if (std::isnan(input))
      input = 0.0f;
    float filtered = filter.processSample(0, input);
    bufferData[i] = filtered;
  }

  if (!adsr.isActive()) {
    clearCurrentNote();
    return;
  }

  for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
    outputBuffer.addFrom(ch, startSample, tempBuffer, 0, 0, numSamples);
  }
}

//==============================================================================
// SynthEngine
//==============================================================================

SynthEngine::SynthEngine() {
  // Add voices
  for (int i = 0; i < 8; ++i) {
    addVoice(new HowlingVoice());
  }
}

void SynthEngine::initialize() {
  // Clears sounds and voices? No, just sounds.
  clearSounds();
}

void SynthEngine::prepare(double sampleRate, int samplesPerBlock) {
  setCurrentPlaybackSampleRate(sampleRate);
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->prepare(sampleRate, samplesPerBlock);
    }
  }
}

// ... (existing updateSampleParams)
void SynthEngine::updateSampleParams(float tune, float sampleStart,
                                     float sampleEnd, bool loop) {
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateSampleParams(tune, sampleStart, sampleEnd, loop);
    }
  }
}

void SynthEngine::updateParams(float attack, float decay, float sustain,
                               float release, float cutoff, float resonance,
                               float lfoRate, float lfoDepth) {
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateADSR(attack, decay, sustain, release);
      voice->updateFilter(cutoff, resonance);
      voice->updateLFO(lfoRate, lfoDepth);
    }
  }
}
