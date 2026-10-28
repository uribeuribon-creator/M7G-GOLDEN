#pragma once
#include "../LookAndFeel/M7GLookAndFeel.h"
#include "../PluginProcessor.h"
namespace freshui {
class FineSlider : public juce::Slider {
public:
  void mouseWheelMove(const juce::MouseEvent &,
                      const juce::MouseWheelDetails &w) override {
    if (w.deltaY != 0)
      setValue(getValue() + (w.deltaY > 0 ? .1 : -.1),
               juce::sendNotificationSync);
  }
};
class Knob final : public juce::Component {
public:
  Knob(FreshProcessor &p, fresh::Param id, juce::String caption,
       juce::String subtitle, juce::Colour color = gold)
      : processor(p), param(id), label(std::move(caption)),
        unit(std::move(subtitle)) {
    auto &s = fresh::specs[size_t(id)];
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 95, 24);
    slider.setRange(s.low, s.high, .01);
    slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity(250);
    slider.setDoubleClickReturnValue(true, s.initial);
    slider.setPopupDisplayEnabled(true, true, this);
    slider.setTooltip(juce::String(s.name) +
                      " - Double click resets the default value");
    slider.getProperties().set("accent", int64_t(color.getARGB()));
    slider.setComponentID(s.id);
    slider.setTitle(s.name);
    addAndMakeVisible(slider);
    attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            p.state, s.id, slider);
  }
  void resized() override {
    slider.setBounds(0, 20, getWidth(), std::max(50, getHeight() - 42));
  }
  void paint(juce::Graphics &g) override {
    M7GLookAndFeel::text(g, label, {0, 0, getWidth(), 20}, 13, white,
                         juce::Justification::centred, true);
    M7GLookAndFeel::text(g, unit, {0, getHeight() - 19, getWidth(), 17}, 10,
                         dim);
  }
  FineSlider slider;

private:
  FreshProcessor &processor;
  fresh::Param param;
  juce::String label, unit;
  std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>
      attachment;
};
class Options final : public juce::Component, private juce::Timer {
public:
  Options(FreshProcessor &p, fresh::Param id, int module)
      : processor(p), param(id) {
    auto names = fresh::choices(id);
    for (int i = 0; i < names.size(); ++i) {
      auto b = std::make_unique<juce::TextButton>(names[i]);
      b->getProperties().set("accent", int64_t(accent(module).getARGB()));
      b->setComponentID(juce::String(fresh::specs[size_t(id)].id) + "-" +
                        juce::String(i));
      b->setTooltip(juce::String(fresh::specs[size_t(id)].name) + ": " +
                    names[i]);
      b->onClick = [this, i] {
        processor.undo.beginNewTransaction(fresh::specs[size_t(param)].name);
        if (param == fresh::eqStyle)
          processor.setEQStyle(i);
        else
          processor.set(param, float(i));
      };
      addAndMakeVisible(*b);
      buttons.push_back(std::move(b));
    }
    startTimerHz(20);
    timerCallback();
  }
  void resized() override {
    int unit = getWidth() / std::max(1, int(buttons.size()));
    for (size_t i = 0; i < buttons.size(); ++i)
      buttons[i]->setBounds(int(i) * unit, 0, unit - 6, getHeight());
  }

private:
  void timerCallback() override {
    int selected = int(processor.settings()[size_t(param)]);
    for (size_t i = 0; i < buttons.size(); ++i)
      buttons[i]->setToggleState(int(i) == selected,
                                 juce::dontSendNotification);
  }
  FreshProcessor &processor;
  fresh::Param param;
  std::vector<std::unique_ptr<juce::TextButton>> buttons;
};
class ModuleContent : public juce::Component {
public:
  ModuleContent(FreshProcessor &p, int id, juce::String name, fresh::Param on)
      : processor(p), module(id), title(std::move(name)) {
    title.getProperties().set("module-title", true);
    title.setClickingTogglesState(true);
    title.setComponentID(juce::String(fresh::specs[size_t(on)].id));
    title.setTooltip("Click the module title to enable or bypass this section");
    addAndMakeVisible(title);
    attachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            p.state, fresh::specs[size_t(on)].id, title);
  }
  void paint(juce::Graphics &g) override {
    g.fillAll(panel);
    g.setColour(juce::Colour(0xff292925));
    g.drawHorizontalLine(67, 28.f, float(getWidth() - 28));
  }
  void resized() override { title.setBounds(30, 19, getWidth() - 60, 35); }
  virtual void setExpert(bool value) {
    expert = value;
    resized();
  }

protected:
  FreshProcessor &processor;
  int module;
  bool expert = false;
  juce::TextButton title;

private:
  std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>
      attachment;
};
} // namespace freshui
