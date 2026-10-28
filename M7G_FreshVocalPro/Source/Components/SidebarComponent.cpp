#include "SidebarComponent.h"
namespace freshui {
class SidebarComponent::Entry final : public juce::TextButton,
                                      public juce::DragAndDropTarget {
public:
  Entry(SidebarComponent &owner, int index) : sidebar(owner), id(index) {
    const char *names[]{"Dynamics", "EQ", "Space", "Character"};
    setButtonText(names[index]);
    setComponentID("section-" + juce::String(index));
    onClick = [this] {
      sidebar.processor.selectedTab.store(id);
      if (sidebar.onSelect)
        sidebar.onSelect(id);
    };
  }
  void mouseDrag(const juce::MouseEvent &e) override {
    if (sidebar.processor.viewMode.load() == 2 &&
        e.getDistanceFromDragStart() > 7 && !sidebar.isDragAndDropActive())
      sidebar.startDragging("fresh-module:" + juce::String(id), this);
  }
  bool isInterestedInDragSource(const SourceDetails &d) override {
    return sidebar.processor.viewMode.load() == 2 &&
           d.description.toString().startsWith("fresh-module:");
  }
  void itemDropped(const SourceDetails &d) override {
    int source = d.description.toString()
                     .fromFirstOccurrenceOf(":", false, false)
                     .getIntValue();
    if (source >= 0 && source < 4)
      sidebar.processor.reorder(source, id);
  }
  void paintButton(juce::Graphics &g, bool over, bool down) override {
    getLookAndFeel().drawButtonBackground(g, *this, gold, over, down);
    M7GLookAndFeel::text(
        g, getButtonText(), getLocalBounds().withTrimmedLeft(38), 15,
        getToggleState() ? white : dim, juce::Justification::left, true);
    g.setColour(getToggleState() ? white : juce::Colour(0xff4d4d48));
    g.fillEllipse(9.f, float(getHeight() / 2 - 2), 4, 4);
    juce::Path icon;
    float x = 22, y = getHeight() * .5f;
    if (id == 0) {
      icon.startNewSubPath(x - 4, y + 7);
      icon.lineTo(x + 6, y - 7);
      icon.lineTo(x + 9, y - 7);
    }
    if (id == 1) {
      icon.startNewSubPath(x - 5, y + 5);
      icon.quadraticTo(x + 1, y - 12, x + 9, y + 1);
    }
    if (id == 2) {
      for (int i = 0; i < 3; ++i) {
        g.drawEllipse(x - 7 + i * 2, y - 7 + i * 2, 14.f - i * 4, 14.f - i * 4,
                      1);
      }
    }
    if (id == 3) {
      icon.startNewSubPath(x - 5, y + 6);
      icon.lineTo(x - 1, y - 5);
      icon.lineTo(x + 5, y + 5);
      icon.lineTo(x + 10, y - 6);
    }
    g.strokePath(icon, juce::PathStrokeType(1.5f));
  }

private:
  SidebarComponent &sidebar;
  int id;
};
SidebarComponent::SidebarComponent(FreshProcessor &p) : processor(p) {
  for (int i = 0; i < 4; ++i) {
    entries[size_t(i)] = std::make_unique<Entry>(*this, i);
    addAndMakeVisible(*entries[size_t(i)]);
  }
  startTimerHz(20);
}
SidebarComponent::~SidebarComponent() = default;
void SidebarComponent::resized() {
  auto order = fresh::orders()[size_t(
      juce::jlimit(0, 23, int(processor.settings()[fresh::order])))];
  if (processor.viewMode.load() != 2)
    order = {{0, 1, 2, 3}};
  for (int i = 0; i < 4; ++i)
    entries[size_t(order[size_t(i)])]->setBounds(10, 30 + i * 84,
                                                 getWidth() - 20, 70);
}
void SidebarComponent::timerCallback() {
  int selected = processor.selectedTab.load();
  for (int i = 0; i < 4; ++i)
    entries[size_t(i)]->setToggleState(i == selected,
                                       juce::dontSendNotification);
  int order = int(processor.settings()[fresh::order]),
      mode = processor.viewMode.load();
  if (order != lastOrder || mode != lastMode) {
    lastOrder = order;
    lastMode = mode;
    resized();
    repaint();
  }
}
void SidebarComponent::paint(juce::Graphics &g) {
  g.fillAll(juce::Colour(0xff0f0f0f));
  g.setColour(juce::Colour(0xff292923));
  g.drawVerticalLine(getWidth() - 1, 0.f, float(getHeight()));
  M7GLookAndFeel::text(g, "VOCAL ENGINE", {10, 0, getWidth() - 20, 25}, 10,
                       dim);
  if (processor.viewMode.load() == 2)
    M7GLookAndFeel::text(g, "ARRASTRA MODULOS\nPARA CAMBIAR EL ORDEN",
                         {12, 390, getWidth() - 24, 50}, 10, gold);
}
} // namespace freshui
