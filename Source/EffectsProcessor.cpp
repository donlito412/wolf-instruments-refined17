#include "EffectsProcessor.h"

EffectsProcessor::EffectsProcessor() {
  // Initialize Waveshaper with tanh transfer function
  distortion.functionToUse = [](float x) { return std::tanh(x); };

  // Delay setup
  delayLine.setMaximumDelayInSamples(44100 * 2);
}

EffectsProcessor::~EffectsProcessor() {}

void EffectsProcessor::prepare(juce::dsp::ProcessSpec &spec) {
  currentSampleRate = spec.sampleRate;

  // Prepare Distortion
  distortion.prepare(spec);

  // Prepare Delay
  delayLine.prepare(spec);
  // Important: Set correct max delay based on sample rate
  delayLine.setMaximumDelayInSamples(
      static_cast<int>(spec.sampleRate * maxDelayTime));

  // Allocate mix buffer or feedback storage?
  // juce::dsp::DelayLine handles internal storage.

  delayLine.reset();

  // Prepare Reverb
  reverb.prepare(spec);
  reverb.reset();
}

void EffectsProcessor::reset() {
  distortion.reset();
  delayLine.reset();
  reverb.reset();
}

void EffectsProcessor::updateParameters(float distDrive, float distMix,
                                        float delayTime, float delayFeedback,
                                        float delayMix, float reverbSize,
                                        float reverbDamping, float reverbMix) {
  distDriveParam = distDrive;
  distMixParam = distMix;

  delayTimeParam = delayTime;
  delayFeedbackParam = delayFeedback;
  delayMixParam = delayMix;

  // Map Reverb params
  reverbParams.roomSize = reverbSize;
  reverbParams.damping = reverbDamping;
  reverbParams.wetLevel = reverbMix;
  reverbParams.dryLevel = 1.0f - reverbMix;
  reverbParams.width = 1.0f;
  reverbParams.freezeMode = 0.0f;

  reverb.setParameters(reverbParams);
}

void EffectsProcessor::process(juce::AudioBuffer<float> &buffer) {
  juce::ScopedNoDenormals noDenormals;

  auto totalNumInputChannels = buffer.getNumChannels();
  auto numSamples = buffer.getNumSamples();

  // ===========================================================================
  // 1. Distortion (Soft Clipping)
  // ===========================================================================
  // We apply Pre-Gain -> Tanh -> Mix
  // Even if Mix is 0, we can skip processing to save CPU, but for now we run it
  // if mix > 0.

  if (distMixParam > 0.0f) {
    // Apply Drive (Gain)
    // Drive 0.0 -> 1.0 Gain (Unity)
    // Drive 1.0 -> 50.0 Gain (+34dB)
    float gain = 1.0f + (distDriveParam * 49.0f);

    // We need to process purely wet for the wet path?
    // Or process in-place and mix?
    // Let's do Channel-by-Channel for simplicity and correct mixing.

    for (int ch = 0; ch < totalNumInputChannels; ++ch) {
      auto *data = buffer.getWritePointer(ch);
      for (int i = 0; i < numSamples; ++i) {
        float dry = data[i];
        float driven = dry * gain;
        float wet = std::tanh(driven);

        // Mix
        data[i] = (dry * (1.0f - distMixParam)) + (wet * distMixParam);
      }
    }
  }

  // ===========================================================================
  // 2. Delay
  // ===========================================================================
  if (delayMixParam > 0.0f) {
    // Calculate Delay Time in Samples
    float delaySamples = delayTimeParam * (float)currentSampleRate;
    // Clamp to valid range (1 sample to max) -> strict clamp
    delaySamples = juce::jlimit(1.0f, (float)(maxDelayTime * currentSampleRate),
                                delaySamples);

    // Set Delay
    delayLine.setDelay(delaySamples);

    for (int ch = 0; ch < totalNumInputChannels; ++ch) {
      auto *channelData = buffer.getWritePointer(ch);
      for (int i = 0; i < numSamples; ++i) {
        float input = channelData[i];

        // Standard Feedback Delay:
        // 1. Read delayed signal (pop)
        // Pass -1.0f to use the setDelay() value
        float delayedSignal = delayLine.popSample(ch, -1.0f);

        // 2. Calculate what to push back (Feedback)
        // Feedback loop: Input + (Delayed * Feedback)
        float feedbackSignal = input + (delayedSignal * delayFeedbackParam);

        // 3. Push to delay line
        delayLine.pushSample(ch, feedbackSignal);

        // 4. Output Mixer: Dry + (Delayed * Wet)
        channelData[i] = input + (delayedSignal * delayMixParam);
      }
    }
  }

  // ===========================================================================
  // 3. Reverb
  // ===========================================================================
  if (reverbMixParam > 0.0f) {
    // JUCE Reverb handles mixing via parameters (wetLevel, dryLevel).
    // So we just process in place.
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
  }
}
