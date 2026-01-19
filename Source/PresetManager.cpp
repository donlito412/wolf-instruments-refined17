#include "PresetManager.h"

const juce::File PresetManager::defaultDirectory{
    juce::File::getSpecialLocation(juce::File::userMusicDirectory)
        .getChildFile("Wolf Instruments")
        .getChildFile("Presets")};

const juce::String PresetManager::presetExtension{".xml"};

PresetManager::PresetManager(juce::AudioProcessorValueTreeState &apvts)
    : valueTreeState(apvts) {
  // Create default directory if it doesn't exist
  if (!defaultDirectory.exists()) {
    const auto result = defaultDirectory.createDirectory();
    if (result.failed()) {
      DBG("Could not create preset directory: " + result.getErrorMessage());
    }
  }

  // Ensure Category Subfolders exist
  juce::StringArray categories = {"Bass", "Leads",    "Pads",
                                  "Keys", "Plucks",   "Drums",
                                  "FX",   "Textures", "Sequence"};
  for (const auto &cat : categories) {
    auto catDir = defaultDirectory.getChildFile(cat);
    if (!catDir.exists())
      catDir.createDirectory();
  }
}

void PresetManager::savePreset(const juce::String &presetName,
                               const juce::String &category) {
  if (presetName.isEmpty())
    return;

  auto state = valueTreeState.copyState();
  state.setProperty("Category", category, nullptr);

  const auto xml = state.createXml();

  // Save to Category Folder (or root if "All")
  juce::File folder = defaultDirectory;
  if (category != "All") {
    folder = defaultDirectory.getChildFile(category);
    if (!folder.exists())
      folder.createDirectory();
  }

  const auto presetFile = folder.getChildFile(presetName + presetExtension);

  if (!xml->writeTo(presetFile)) {
    DBG("Could not create preset file: " + presetFile.getFullPathName());
  }
  currentPresetName = presetName;
}

void PresetManager::deletePreset(const juce::String &presetName) {
  if (presetName.isEmpty())
    return;

  // Search in all folders
  const auto presetFile = getPresetFile(presetName);

  if (!presetFile.existsAsFile()) {
    DBG("Preset file " + presetName + " does not exist");
    return;
  }

  if (!presetFile.deleteFile()) {
    DBG("Preset file " + presetFile.getFullPathName() +
        " could not be deleted");
    return;
  }

  currentPresetName = "";
}

void PresetManager::loadPreset(const juce::String &presetName) {
  if (presetName.isEmpty())
    return;

  const auto presetFile = getPresetFile(presetName);

  if (!presetFile.existsAsFile()) {
    DBG("Preset file " + presetName + " does not exist");
    return;
  }

  // Load XML
  const auto xml = juce::parseXML(presetFile);
  if (xml != nullptr) {
    // Replace state
    valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
    currentPresetName = presetName;
  }
}

// Helper to find file across subfolders
juce::File PresetManager::getPresetFile(const juce::String &presetName) const {
  auto file = defaultDirectory.getChildFile(presetName + presetExtension);
  if (file.existsAsFile())
    return file;

  // Scan subfolders
  auto subDirs =
      defaultDirectory.findChildFiles(juce::File::findDirectories, false);
  for (const auto &dir : subDirs) {
    file = dir.getChildFile(presetName + presetExtension);
    if (file.existsAsFile())
      return file;
  }
  return juce::File();
}

int PresetManager::loadNextPreset() {
  const auto allPresets = getAllPresets();
  if (allPresets.isEmpty())
    return -1;

  const auto currentIndex = allPresets.indexOf(currentPresetName);
  const auto nextIndex =
      currentIndex + 1 > allPresets.size() - 1 ? 0 : currentIndex + 1;
  loadPreset(allPresets[nextIndex]);
  return nextIndex;
}

int PresetManager::loadPreviousPreset() {
  const auto allPresets = getAllPresets();
  if (allPresets.isEmpty())
    return -1;

  const auto currentIndex = allPresets.indexOf(currentPresetName);
  const auto prevIndex =
      currentIndex - 1 < 0 ? allPresets.size() - 1 : currentIndex - 1;
  loadPreset(allPresets[prevIndex]);
  return prevIndex;
}

juce::StringArray PresetManager::getAllPresets() const {
  juce::StringArray presets;

  // Scan Root
  auto fileArray = defaultDirectory.findChildFiles(
      juce::File::TypesOfFileToFind::findFiles, false, "*" + presetExtension);
  for (const auto &file : fileArray) {
    presets.add(file.getFileNameWithoutExtension());
  }

  // Scan Subfolders
  auto subDirs =
      defaultDirectory.findChildFiles(juce::File::findDirectories, false);
  for (const auto &dir : subDirs) {
    fileArray =
        dir.findChildFiles(juce::File::findFiles, false, "*" + presetExtension);
    for (const auto &file : fileArray) {
      presets.add(file.getFileNameWithoutExtension());
    }
  }

  return presets;
}

juce::String PresetManager::getCurrentPreset() const {
  return currentPresetName;
}

juce::File PresetManager::getPresetFolder() const { return defaultDirectory; }
