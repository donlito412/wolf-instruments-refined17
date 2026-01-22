#pragma once

#include <JuceHeader.h>

class EffectsProcessor {
public:
  EffectsProcessor();
  ~EffectsProcessor();

  void prepare(juce::dsp::ProcessSpec &spec);
  void process(juce::AudioBuffer<float> &buffer);
  void reset();

  void updateParameters(float distDrive, float distMix, float delayTime,
                        float delayFeedback, float delayMix, float reverbSize,
                        float reverbDamping, float reverbMix);

private:
  // --- Distortion ---
  // Use Waveshaper for standard tanh distortion (Josh Hodge style)
  float distDriveParam = 0.0f;
  float distMixParam = 0.0f;
  juce::dsp::WaveShaper<float> distortion;

  // --- Delay ---
  float delayTimeParam = 0.5f;
  float delayFeedbackParam = 0.3f;
  float delayMixParam = 0.0f;

  // Choose Lagrange for better pitch shifting/modulation support, or Linear for
  // efficiency. Using Linear for standard echo.
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>
      delayLine;
  static constexpr float maxDelayTime = 2.0f;
  std::vector<float>
      delayFeedbackBuffer; // To store feedback values per channel

  // --- Reverb ---
  juce::dsp::Reverb reverb;
  juce::dsp::Reverb::Parameters reverbParams;
  float reverbMixParam = 0.0f;

  double currentSampleRate = 44100.0;

  // Helper for Dry/Wet mixing
  // We'll do simple linear mix implementation inline for clarity
};
