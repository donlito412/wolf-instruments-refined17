#include "PresetBrowser.h"

PresetBrowser::PresetBrowser(PresetManager &pm) : presetManager(pm) {
  addAndMakeVisible(presetList);
  presetList.setModel(this);
  presetList.setColour(juce::ListBox::backgroundColourId,
                       juce::Colours::transparentBlack);
  presetList.setRowHeight(40);

  addAndMakeVisible(titleLabel);
  titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
  titleLabel.setJustificationType(juce::Justification::centred);
  titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

  // Search Box
  addAndMakeVisible(searchBox);
  searchBox.setTextToShowWhenEmpty("Search presets...",
                                   juce::Colours::white.withAlpha(0.5f));
  searchBox.setColour(juce::TextEditor::backgroundColourId,
                      juce::Colour::fromString("FF222222"));
  searchBox.setColour(juce::TextEditor::outlineColourId,
                      juce::Colour::fromString("FF666670"));
  searchBox.onTextChange = [this] { filterPresets(); };

  // Category Filter
  addAndMakeVisible(categoryFilter);
  categoryFilter.addItemList(categories, 1);
  categoryFilter.setSelectedId(1); // "All"
  categoryFilter.setColour(juce::ComboBox::backgroundColourId,
                           juce::Colour::fromString("FF222222"));
  categoryFilter.setColour(juce::ComboBox::textColourId, juce::Colours::white);
  categoryFilter.setColour(juce::ComboBox::arrowColourId,
                           juce::Colour::fromString("FF88CCFF"));
  categoryFilter.setColour(juce::ComboBox::outlineColourId,
                           juce::Colour::fromString("FF666670"));
  categoryFilter.onChange = [this] { filterPresets(); };

  refresh();
}

PresetBrowser::~PresetBrowser() {}

void PresetBrowser::paint(juce::Graphics &g) {
  g.fillAll(
      juce::Colour::fromString("FF111111").withAlpha(0.95f)); // Dark Overlay

  g.setColour(juce::Colour::fromString("FF666670"));
  g.drawRect(getLocalBounds(), 1.0f); // Border
}

void PresetBrowser::resized() {
  auto area = getLocalBounds();

  // Header
  auto header = area.removeFromTop(50);
  titleLabel.setBounds(header.removeFromLeft(header.getWidth() / 3));

  // Search & Filter
  searchBox.setBounds(header.removeFromLeft(header.getWidth() / 2).reduced(5));
  categoryFilter.setBounds(header.reduced(5));

  area.reduce(20, 20);
  presetList.setBounds(area);
}

int PresetBrowser::getNumRows() { return displayedPresets.size(); }

void PresetBrowser::paintListBoxItem(int rowNumber, juce::Graphics &g,
                                     int width, int height,
                                     bool rowIsSelected) {
  if (rowNumber >= displayedPresets.size())
    return;

  if (rowIsSelected) {
    g.setColour(juce::Colour::fromString("FF88CCFF").withAlpha(0.2f));
    g.fillRect(0, 0, width, height);
  }

  g.setColour(rowIsSelected ? juce::Colour::fromString("FF88CCFF")
                            : juce::Colours::white.withAlpha(0.8f));
  g.setFont(16.0f);
  g.drawText(displayedPresets[rowNumber], 5, 0, width - 10, height,
             juce::Justification::centredLeft, true);

  g.setColour(juce::Colours::white.withAlpha(0.1f));
  g.fillRect(0, height - 1, width, 1); // Separator
}

void PresetBrowser::listBoxItemClicked(int rowNumber,
                                       const juce::MouseEvent &) {
  if (rowNumber < 0 || rowNumber >= displayedPresets.size())
    return;

  const auto presetName = displayedPresets[rowNumber];
  presetManager.loadPreset(presetName);

  if (onPresetSelected)
    onPresetSelected();
}

void PresetBrowser::selectedRowsChanged(int) {}

void PresetBrowser::refresh() {
  allPresetsInfo.clear();
  auto files = presetManager.getAllPresets();

  // Scan metadata
  for (const auto &file : files) {
    juce::String name = file.getFileNameWithoutExtension();

    // For WAVs, we don't have metadata yet.
    // In future, we could look for a matching .xml file or check parent folder
    // name as Category. For now, default category to file's parent folder name
    // if inside a subfolder?

    juce::String category = "All";

    // Check if valid file
    if (file.existsAsFile()) {
      // Use parent directory name as category if it's not the root Preset
      // folder
      auto parent = file.getParentDirectory();
      if (parent != presetManager.getPresetFolder() &&
          parent != PresetManager::factoryDirectory) {
        category = parent.getFileName();
      }
    }

    allPresetsInfo.push_back({name, category});
  }

  filterPresets();
}

void PresetBrowser::visibilityChanged() {
  if (isVisible()) {
    searchBox.setText("", juce::dontSendNotification);
    refresh();
  }
}

void PresetBrowser::filterPresets() {
  displayedPresets.clear();
  juce::String searchText = searchBox.getText().toLowerCase();
  juce::String selectedCat = categoryFilter.getText();

  for (const auto &info : allPresetsInfo) {
    bool matchSearch =
        searchText.isEmpty() || info.name.toLowerCase().contains(searchText);
    bool matchCat = (selectedCat == "All") || (info.category == selectedCat);

    if (matchSearch && matchCat) {
      displayedPresets.add(info.name);
    }
  }
  presetList.updateContent();
  repaint();
}
