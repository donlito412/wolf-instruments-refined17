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

  // Prepare crossover filter for Bass (120Hz)
  crossoverFilter.prepare(spec);
  crossoverFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
  crossoverFilter.setCutoffFrequency(120.0f);

  // Resize temp buffer for processing
  tempBuffer.setSize(1, samplesPerBlock); // Mono voice
}

void HowlingVoice::updateFilter(float cutoff, float resonance, int filterType) {
  baseCutoff = cutoff;
  baseResonance = resonance;
  filter.setCutoffFrequency(cutoff);
  filter.setResonance(resonance);

  switch (filterType) {
  case 0:
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    isNotch = false;
    break;
  case 1:
    filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    isNotch = false;
    break;
  case 2:
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    isNotch = false;
    break;
  case 3:
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    isNotch = true;
    break;
  default:
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    isNotch = false;
    break;
  }
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

void HowlingVoice::setPan(float newPan) { pan = newPan; }

void HowlingVoice::startNote(int midiNoteNumber, float velocity,
                             juce::SynthesiserSound *sound,
                             int currentPitchWheelPosition) {
  // Check if it's Bass or One-Shot
  isCurrentSoundBass = false;
  isCurrentSoundOneShot = false;

  if (auto *hs = dynamic_cast<HowlingSound *>(sound)) {
    isCurrentSoundBass = hs->isBassSample();
    isCurrentSoundOneShot = hs->isOneShotSample();
  }

  // 1. Base startNote
  juce::SamplerVoice::startNote(midiNoteNumber, velocity, sound,
                                currentPitchWheelPosition);

  crossoverFilter.reset();

  adsr.noteOn();
  filter.reset();
  lfo.reset();
}

void HowlingVoice::stopNote(float velocity, bool allowTailOff) {
  // If One-Shot, IGNORE stopNote (let sample play to end)
  // SamplerVoice naturally stops when sample data runs out (if not looping).
  if (isCurrentSoundOneShot) {
    return;
  }

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

  // 1. Render Raw Sample
  juce::SamplerVoice::renderNextBlock(tempBuffer, 0, numSamples);

  // 2. ADSR
  adsr.applyEnvelopeToBuffer(tempBuffer, 0, numSamples);

  auto *bufferData = tempBuffer.getWritePointer(0);

  // 3. Filter Processing
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

    if (isNotch) {
      filtered = input - filtered;
    }

    // Safety Check for NaN/Infinity
    if (std::isnan(filtered) || std::isinf(filtered)) {
      filtered = 0.0f;
      filter.reset();
    }

    bufferData[i] = filtered;
  }

  if (!adsr.isActive()) {
    clearCurrentNote();
    return;
  }

  // FORCE STOP if One-Shot and Sample has finished playing
  // SamplerVoice::isVoiceActive() returns false when sample finishes (if not
  // looping)
  if (isCurrentSoundOneShot && !juce::SamplerVoice::isVoiceActive()) {
    clearCurrentNote();
    return;
  }

  // 4. Panning and Output Mix
  if (isCurrentSoundBass) {
    // Bass Logic: Lows (<120Hz) -> Mono, Highs -> Panned

    // We can't easily do per-sample crossover efficiently here without
    // block processing or another temp buffer, but LinkwitzRiley is per-sample
    // capable. However, L-R is usually 2 channels (stereo). Here we have mono
    // voice signal `tempBuffer`. We want to split it: Lows, Highs.

    // Process the whole block through a filter to get Lows?
    // But filters are stateful.

    // Let's use two filters? Or process tempBuffer in place for Lows,
    // and subtract from original to get Highs?
    // (Linkwitz-Riley sums flat).

    // Create a copy for Highs
    juce::AudioBuffer<float> highBuffer;
    highBuffer.makeCopyOf(tempBuffer);

    // Process tempBuffer (Lows)
    juce::dsp::AudioBlock<float> block(tempBuffer);
    // Since we only have 1 channel in tempBuffer
    juce::dsp::ProcessContextReplacing<float> context(block);
    crossoverFilter.process(context);

    // Now tempBuffer contains Lows.
    // Highs = Original (highBuffer) - Lows (tempBuffer)
    highBuffer.addFrom(0, 0, tempBuffer.getReadPointer(0), numSamples,
                       -1.0f); // Subtract

    // Mix to Output
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      // Pan Highs
      float panGain = 1.0f;
      if (outputBuffer.getNumChannels() == 2) {
        float panRad = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        if (ch == 0)
          panGain = std::cos(panRad);
        if (ch == 1)
          panGain = std::sin(panRad);
      }

      // Mono Lows (Center panned -> equal gain 0.707 or 1.0 depending on law,
      // let's use 1.0 for kick/sub power)
      float bassGain = 1.0f;
      // Actually standard constant power center is 0.707, but for sub we often
      // want full power. Let's stick to standard pan center gain approx 0.707
      // if we want consistency with other sounds. But typically "Mono" means
      // equal in both.
      bassGain = (outputBuffer.getNumChannels() == 2) ? 0.707f : 1.0f;

      // Add Lows (Center)
      outputBuffer.addFrom(ch, startSample, tempBuffer, 0, 0, numSamples,
                           bassGain);

      // Add Highs (Panned)
      outputBuffer.addFrom(ch, startSample, highBuffer, 0, 0, numSamples,
                           panGain);
    }
  } else {
    // Standard processing
    for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
      float gain = 1.0f;
      if (outputBuffer.getNumChannels() == 2) {
        float panRad = (pan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        if (ch == 0)
          gain = std::cos(panRad);
        if (ch == 1)
          gain = std::sin(panRad);
      }

      outputBuffer.addFrom(ch, startSample, tempBuffer, 0, 0, numSamples, gain);
    }
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
                               int filterType, float lfoRate, float lfoDepth) {
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateADSR(attack, decay, sustain, release);
      voice->updateFilter(cutoff, resonance, filterType);
      voice->updateLFO(lfoRate, lfoDepth);
    }
  }
}

void SynthEngine::setPackMode(int size, float spread) {
  packSize = size;
  packSpread = spread;
}

void SynthEngine::noteOn(int midiChannel, int midiNoteNumber, float velocity) {
  // Standard note on
  // If Unison is active (packSize > 1), trigger multiple voices

  if (packSize > 1 && packSpread > 0.0f) {
    // Basic Unison: Trigger extra voices with detune
    // Since SamplerVoice doesn't support fine detune easily without pitch ratio
    // hack, we might just trigger neighbor notes or same note multiple times if
    // we have voice stealing? Standard JUCE Synthesiser handles note
    // allocation. For now, just call base.
    juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
  } else {
    juce::Synthesiser::noteOn(midiChannel, midiNoteNumber, velocity);
  }
}
