#pragma once
#include "../ThirdParty/rubberband/rubberband/RubberBandStretcher.h"
#include "AudioMemory.h"
#include <memory>
namespace scratch {
class DigitalStretch {
public:
  static constexpr int Block = 128, Padding = 4096, FifoSize = 65536;
  static int paddingFor(double sr) {
    return std::clamp(int(std::ceil(sr / 48000. * 4096 / Block)) * Block, 2048,
                      16384);
  }
  int latencyPadding() const { return padding; }
  void prepare(double sr) {
    padding = paddingFor(sr);
    using RB = RubberBand::RubberBandStretcher;
    auto options = RB::OptionProcessRealTime | RB::OptionEngineFaster |
                   RB::OptionThreadingNever | RB::OptionWindowShort |
                   RB::OptionPitchHighConsistency | RB::OptionTransientsSmooth |
                   RB::OptionDetectorSoft | RB::OptionPhaseIndependent |
                   RB::OptionChannelsTogether;
    stretcher = std::make_unique<RB>(size_t(sr), 2, options, 1., 1.);
    stretcher->setMaxProcessSize(1024);
    reset();
  }
  void reset() {
    stretcher->reset();
    stretcher->setTimeRatio(1);
    stretcher->setPitchScale(1);
    read = 0;
    write = padding;
    underflows = overflows = 0;
    discard = int(stretcher->getStartDelay());
    for (auto &b : fifo)
      b.fill(0);
    input[0].fill(0);
    input[1].fill(0);
    const float *ptr[2]{input[0].data(), input[1].data()};
    int pad = int(stretcher->getPreferredStartPad());
    while (pad > 0) {
      int n = std::min(pad, 1024);
      stretcher->process(ptr, size_t(n), false);
      drain();
      pad -= n;
    }
  }
  // Unit-spaced source frames plus a stretch ratio: pitch never follows platter
  // rate.
  void block(const WordSnapshot *source, double begin, double movement,
             bool sounding, std::array<std::array<float, Block>, 2> &out) {
    float speed = std::abs(float(movement / Block));
    bool moving = sounding && source && speed >= .23f;
    int count = moving
                    ? std::clamp(int(std::round(std::abs(movement))), 32, 512)
                    : Block;
    double direction = movement < 0 ? -1. : 1.;
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < count; ++i)
        input[size_t(c)][size_t(i)] =
            moving ? source->read(begin + direction * i, c, 1, true) : 0;
    double servo =
        std::clamp(1. + (padding - int(write - read)) * .000025, .94, 1.06);
    stretcher->setTimeRatio(
        std::clamp((moving ? double(Block) / count : 1.) * servo, .25, 4.));
    const float *ptr[2]{input[0].data(), input[1].data()};
    stretcher->process(ptr, size_t(count), false);
    drain();
    for (int i = 0; i < Block; ++i) {
      if (read < write) {
        for (int c = 0; c < 2; ++c)
          out[size_t(c)][size_t(i)] = fifo[size_t(c)][size_t(read % FifoSize)];
        ++read;
      } else {
        ++underflows;
        for (int c = 0; c < 2; ++c)
          out[size_t(c)][size_t(i)] = 0;
      }
    }
  }
  int underflows = 0, overflows = 0;
  int queued() const { return int(write - read); }

private:
  void drain() {
    float *out[2]{retrieved[0].data(), retrieved[1].data()};
    int available = stretcher->available();
    while (available > 0) {
      int n = int(stretcher->retrieve(out, size_t(std::min(available, 4096))));
      if (!n)
        break;
      int start = std::min(n, discard);
      discard -= start;
      for (int i = start; i < n; ++i) {
        if (write - read >= FifoSize - 1) {
          ++overflows;
          break;
        }
        for (int c = 0; c < 2; ++c)
          fifo[size_t(c)][size_t(write % FifoSize)] =
              retrieved[size_t(c)][size_t(i)];
        ++write;
      }
      available = stretcher->available();
    }
  }
  std::unique_ptr<RubberBand::RubberBandStretcher> stretcher;
  std::array<std::array<float, FifoSize>, 2> fifo{};
  std::array<std::array<float, 1024>, 2> input{};
  std::array<std::array<float, 4096>, 2> retrieved{};
  int64_t read = 0, write = 0;
  int discard = 0, padding = Padding;
};
} // namespace scratch
