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
  // Simple modulation could happen here, or per-block
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

void HowlingVoice::startNote(int midiNoteNumber, float velocity,
                             juce::SynthesiserSound *sound,
                             int currentPitchWheelPosition) {
  juce::SamplerVoice::startNote(midiNoteNumber, velocity, sound,
                                currentPitchWheelPosition);
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
  // 1. Generate Sample Playback (Standard JUCE Sampler)
  // 2. Post-Processing (Filter)
  // Only process if the voice is active
  if (!isVoiceActive())
    return;

  // Apply Filter
  // We process the buffer in-place.
  // Note: SamplerVoice adds to the buffer, so we need to be careful not to
  // filter the whole buffer if other voices already wrote to it? Actually,
  // renderNextBlock usually *adds* to the buffer. If we filter, we affect the
  // mix. CORRECT APPROACH: Render to a temporary buffer, filter it, then add to
  // output? OR: Filter the *outputBuffer*? No, that filters everything. Since
  // we are inheriting SamplerVoice, we don't have an easy "render into temp"
  // hook unless we override renderNextBlock completely, which defeats the
  // purpose of "simple sampler". BUT: Josh Hodge's tutorial usually uses the
  // Synthesiser's buffer which sums voices. If we want per-voice filtering, we
  // MUST render to temp.

  // Synthesiser's buffer which sums voices. If we want per-voice filtering, we
  // MUST render to temp.

  // Using member tempBuffer (pre-allocated)
  // juce::AudioBuffer<float> tempBuffer;
  // tempBuffer.setSize... (Already handled in prepare and check below)

  if (tempBuffer.getNumSamples() < numSamples)
    tempBuffer.setSize(1, numSamples, false, false, true);

  tempBuffer.clear();

  // Challenge: SamplerVoice::renderNextBlock ADDS to the buffer passed to it.
  // If we pass 'outputBuffer', it mixes in. We can't filter just this voice's
  // contribution easily after. We must pass a fresh temp buffer to
  // SamplerVoice::renderNextBlock.

  // Let's re-call renderNextBlock but with temp buffer?
  // We can't call base class twice.
  // So:
  // 1. Create temp buffer.
  // 2. Call Base::renderNextBlock(tempBuffer).
  // 3. Filter tempBuffer.
  // 4. Add tempBuffer to outputBuffer.

  // However, SamplerVoice::renderNextBlock uses 'startSample'.
  // We should use (tempBuffer, 0, numSamples).

  // Ensure temp buffer is big enough (safety check)
  if (tempBuffer.getNumSamples() < numSamples)
    tempBuffer.setSize(1, numSamples, false, false, true);

  tempBuffer.clear();

  // Call Base
  juce::SamplerVoice::renderNextBlock(tempBuffer, 0, numSamples);

  // Apply ADSR
  adsr.applyEnvelopeToBuffer(tempBuffer, 0, numSamples);

  // Check if voice finished
  if (!adsr.isActive()) {
    clearCurrentNote();
    return;
  }

  // Filter
  // Process Modulations (LFO -> Cutoff)
  // We'll do block-based modulation for efficiency
  // Update Filter params?
  // Actually, let's just run the filter.
  // Need to get valid audio context block.
  juce::dsp::AudioBlock<float> block(tempBuffer);
  juce::dsp::ProcessContextReplacing<float> context(block);
  filter.process(context);

  // Mix into main output
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

void SynthEngine::updateParams(float attack, float decay, float sustain,
                               float release, float cutoff, float resonance,
                               float lfoRate, float lfoDepth) {
  // Update Voices (Filter/LFO)
  for (int i = 0; i < getNumVoices(); ++i) {
    if (auto *voice = dynamic_cast<HowlingVoice *>(getVoice(i))) {
      voice->updateFilter(cutoff, resonance);
      voice->updateLFO(lfoRate, lfoDepth);
      voice->updateADSR(attack, decay, sustain, release);
    }
  }

  // Update Sounds (ADSR) - No longer needed as Voice handles it.
  /*
  for (int i = 0; i < getNumSounds(); ++i) {
      if (auto *sound = dynamic_cast<HowlingSound *>(getSound(i).get())) {
          // sound->setEnvelopeAttackTime(attack);
          // sound->setEnvelopeReleaseTime(release);
      }
  }
  */
}
