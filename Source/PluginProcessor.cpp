#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
HowlingWolvesAudioProcessor::HowlingWolvesAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout()),
      presetManager(apvts), sampleManager(synthEngine) {
  // Load initial samples
  sampleManager.loadSamples();
}

HowlingWolvesAudioProcessor::~HowlingWolvesAudioProcessor() {}

//==============================================================================
const juce::String HowlingWolvesAudioProcessor::getName() const {
  return JucePlugin_Name;
}

bool HowlingWolvesAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
  return true;
#else
  return false;
#endif
}

bool HowlingWolvesAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
  return true;
#else
  return false;
#endif
}

bool HowlingWolvesAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
  return true;
#else
  return false;
#endif
}

double HowlingWolvesAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int HowlingWolvesAudioProcessor::getNumPrograms() {
  return 1; // NB: some hosts don't cope very well if you tell them there are 0
            // programs, so this should be at least 1, even if you're not really
            // implementing programs.
}

int HowlingWolvesAudioProcessor::getCurrentProgram() { return 0; }

void HowlingWolvesAudioProcessor::setCurrentProgram(int index) {}

const juce::String HowlingWolvesAudioProcessor::getProgramName(int index) {
  return {};
}

void HowlingWolvesAudioProcessor::changeProgramName(
    int index, const juce::String &newName) {}

//==============================================================================
void HowlingWolvesAudioProcessor::prepareToPlay(double sampleRate,
                                                int samplesPerBlock) {
  synthEngine.setCurrentPlaybackSampleRate(sampleRate);
}

void HowlingWolvesAudioProcessor::releaseResources() {
  // When playback stops, you can use this as an opportunity to free up any
  // spare memory, etc.
}

bool HowlingWolvesAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const {
  if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
      layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    return false;

  return true;
}

void HowlingWolvesAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                               juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  auto totalNumInputChannels = getTotalNumInputChannels();
  auto totalNumOutputChannels = getTotalNumOutputChannels();

  for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // Process keyboard state
  keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(),
                                      true);

  // Update Parameters from APVTS
  auto *attack = apvts.getRawParameterValue("attack");
  auto *decay = apvts.getRawParameterValue("decay");
  auto *sustain = apvts.getRawParameterValue("sustain");
  auto *release = apvts.getRawParameterValue("release");
  auto *gain = apvts.getRawParameterValue("gain");

  if (attack && decay && sustain && release) {
    synthEngine.updateADSR(attack->load(), decay->load(), sustain->load(),
                           release->load());
  }

  // Process synth
  synthEngine.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

  // Apply Master Gain
  if (gain)
    buffer.applyGain(gain->load());

  // Send to Visualizer
  if (audioVisualizerHook)
    audioVisualizerHook(buffer);
}

//==============================================================================
bool HowlingWolvesAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor *HowlingWolvesAudioProcessor::createEditor() {
  return new HowlingWolvesAudioProcessorEditor(*this);
}

//==============================================================================
void HowlingWolvesAudioProcessor::getStateInformation(
    juce::MemoryBlock &destData) {
  auto state = apvts.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void HowlingWolvesAudioProcessor::setStateInformation(const void *data,
                                                      int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState.get() != nullptr)
    if (xmlState->hasTagName(apvts.state.getType()))
      apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessorValueTreeState::ParameterLayout
HowlingWolvesAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // Example: Master Gain
  layout.add(std::make_unique<juce::AudioParameterFloat>("gain", "Gain", 0.0f,
                                                         1.0f, 0.5f));

  // Attack, Decay, Sustain, Release (Simple ADSR for now, can be expanded)
  layout.add(std::make_unique<juce::AudioParameterFloat>("attack", "Attack",
                                                         0.01f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("decay", "Decay",
                                                         0.01f, 5.0f, 0.1f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain",
                                                         0.0f, 1.0f, 1.0f));
  layout.add(std::make_unique<juce::AudioParameterFloat>("release", "Release",
                                                         0.01f, 5.0f, 0.1f));

  return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new HowlingWolvesAudioProcessor();
}
