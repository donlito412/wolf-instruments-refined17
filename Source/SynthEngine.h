#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    A sound that holds the sample data.
    Wrapper around juce::SamplerSound to allow for custom expansion if needed,
    and to maintain the "HowlingSound" type name used in the codebase.
*/
class HowlingSound : public juce::SamplerSound {
public:
  HowlingSound(const juce::String &name, juce::AudioFormatReader &source,
               const juce::BigInteger &midiNotes, int midiNoteForNormalPitch,
               double attackTimeSecs, double releaseTimeSecs,
               double maxSampleLengthSeconds)
      : juce::SamplerSound(name, source, midiNotes, midiNoteForNormalPitch,
                           attackTimeSecs, releaseTimeSecs,
                           maxSampleLengthSeconds) {}
};

//==============================================================================
/**
    A voice that plays back the HowlingSound (Sample).
    Inherits from juce::SamplerVoice to handle pitch shifting and resampling.
    Adds custom Filter and LFO processing.
*/
class HowlingVoice : public juce::SamplerVoice {
public:
  HowlingVoice();

  bool canPlaySound(juce::SynthesiserSound *sound) override {
    return dynamic_cast<juce::SamplerSound *>(sound) != nullptr;
  }

  // DSP Parameters
  void updateFilter(float cutoff, float resonance);
  void updateLFO(float rate, float depth);
  void prepare(double sampleRate, int samplesPerBlock);

  // Overrides for ADSR control
  void startNote(int midiNoteNumber, float velocity,
                 juce::SynthesiserSound *sound,
                 int currentPitchWheelPosition) override;
  void stopNote(float velocity, bool allowTailOff) override;

  // New Sample Params
  void updateSampleParams(float tune, float sampleStart, float sampleEnd,
                          bool loop);

  // Override render to add post-processing (Filter)
  void renderNextBlock(juce::AudioBuffer<float> &outputBuffer, int startSample,
                       int numSamples) override;

  // Custom ADSR access
  void updateADSR(float attack, float decay, float sustain, float release);

private:
  juce::dsp::StateVariableTPTFilter<float> filter;
  juce::dsp::Oscillator<float> lfo; // For filter modulation
  float lfoDepth = 0.0f;

  juce::ADSR adsr;
  juce::ADSR::Parameters adsrParams;
  float lfoRate = 0.0f;

  // Sample Parameters
  float tuneSemitones = 0.0f;
  float sampleStartPercent = 0.0f;
  float sampleEndPercent = 1.0f;
  bool isLooping = true;

  // Base parameters for modulation
  float baseCutoff = 20000.0f;
  float baseResonance = 0.1f;

  juce::AudioBuffer<float> tempBuffer;

  JUCE_LEAK_DETECTOR(HowlingVoice)
};

//==============================================================================
/**
    The main synthesizer engine.
    Manages voices and sounds.
*/
class SynthEngine : public juce::Synthesiser {
public:
  SynthEngine();

  void initialize();
  void prepare(double sampleRate, int samplesPerBlock);

  void updateParams(float attack, float decay, float sustain, float release,
                    float cutoff, float resonance, float lfoRate,
                    float lfoDepth);

  void updateSampleParams(float tune, float sampleStart, float sampleEnd,
                          bool loop);
};
