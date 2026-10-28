#include "ARA.h"
// Lectura anticipada del evento fuera del callback; cada region conserva su
// cadena y parametros.
namespace fresh {
Renderer::Renderer(ARA::PlugIn::DocumentController *doc, Controller &c)
    : juce::ARAPlaybackRenderer(doc), controller(c) {
  thread.startThread(juce::Thread::Priority::normal);
}
Renderer::~Renderer() {
  releaseResources();
  thread.stopThread(-1);
}
void Renderer::releaseResources() {
  voices.clear();
  readers.clear();
}
void Renderer::prepareToPlay(double sr, int block, int count,
                             juce::AudioProcessor::ProcessingPrecision,
                             AlwaysNonRealtime) {
  releaseResources();
  rate = sr;
  maximum = std::max(32, block);
  channels = count;
  expected = std::numeric_limits<int64_t>::min();
  for (auto *r : getPlaybackRegions()) {
    auto *source = r->getAudioModification()->getAudioSource();
    if (readers.find(source) == readers.end()) {
      Source s;
      s.rate = source->getSampleRate();
      s.reader = std::make_unique<juce::BufferingAudioReader>(
          new juce::ARAAudioSourceReader(source), thread,
          std::max(32768, int(s.rate * 2)));
      s.pcm.setSize(juce::jlimit(1, 2, int(source->getChannelCount())),
                    std::max(8192, int(maximum * s.rate / rate) + 80));
      if (std::abs(s.rate - rate) > .01) {
        s.sinc.resize(1024);
        double cutoff = .94 * std::min(1., rate / s.rate);
        for (int phase = 0; phase < 1024; ++phase) {
          double sum = 0;
          for (int j = 0; j < 64; ++j) {
            double x = j - 31 - double(phase) / 1024;
            double weight =
                (std::abs(x) < 1e-10
                     ? cutoff
                     : std::sin(juce::MathConstants<double>::pi * x * cutoff) /
                           (juce::MathConstants<double>::pi * x)) *
                (.42 -
                 .5 * std::cos(2 * juce::MathConstants<double>::pi * j / 63) +
                 .08 * std::cos(4 * juce::MathConstants<double>::pi * j / 63));
            s.sinc[size_t(phase)][size_t(j)] = float(weight);
            sum += weight;
          }
          for (auto &w : s.sinc[size_t(phase)])
            w /= float(sum);
        }
      }
      s.reader->setReadTimeout(2000);
      s.reader->read(
          &s.pcm, 0, 512,
          std::max<int64_t>(0, r->getStartInAudioModificationSamples()), true,
          true);
      s.reader->setReadTimeout(0);
      readers.emplace(source, std::move(s));
    }
    Voice v;
    v.chain = std::make_unique<Chain>();
    v.chain->prepare(sr, count, maximum);
    v.audio.setSize(count, maximum);
    voices.emplace(r, std::move(v));
  }
}
bool Renderer::processBlock(
    juce::AudioBuffer<float> &b, juce::AudioProcessor::Realtime realtime,
    const juce::AudioPlayHead::PositionInfo &p) noexcept {
  juce::ScopedTryReadLock guard(controller.modelLock);
  if (!guard.isLocked()) {
    b.clear();
    return true;
  }
  auto position = p.getTimeInSamples();
  b.clear();
  if (!position.hasValue())
    return false;
  if (!p.getIsPlaying()) {
    expected = std::numeric_limits<int64_t>::min();
    return true;
  }
  bool seek = *position != expected;
  expected = *position + b.getNumSamples();
  double bpm = p.getBpm().orFallback(120.);
  bool success = true;
  for (auto *region : getPlaybackRegions()) {
    auto found = voices.find(region);
    if (found == voices.end())
      continue;
    auto &voice = found->second;
    if (seek)
      voice.chain->reset();
    auto *mod = region->getAudioModification<Modification>();
    auto reader = readers.find(mod->getAudioSource());
    if (reader == readers.end())
      continue;
    auto values = mod->parameters.load();
    double start = region->getStartInPlaybackTime(),
           end = region->getEndInPlaybackTime();
    double ratio = region->getDurationInPlaybackTime() > 0
                       ? region->getDurationInAudioModificationTime() /
                             region->getDurationInPlaybackTime()
                       : 1;
    for (int offset = 0; offset < b.getNumSamples(); offset += maximum) {
      int n = std::min(maximum, b.getNumSamples() - offset);
      double at = (*position + offset) / rate;
      if (at >= end + 12 || at + n / rate <= start)
        continue;
      voice.audio.clear();
      auto &source = reader->second;
      int64_t first =
          int64_t(std::floor((region->getStartInAudioModificationTime() +
                              (at - start) * ratio) *
                             source.rate)) -
          (source.sinc.empty() ? 1 : 31);
      int needed = int(std::ceil(n * source.rate / rate * ratio)) +
                   (source.sinc.empty() ? 4 : 66);
      bool ready = false;
      if (needed <= source.pcm.getNumSamples() && at < end) {
        source.reader->setReadTimeout(
            realtime == juce::AudioProcessor::Realtime::no ? 2000 : 0);
        ready = source.reader->read(&source.pcm, 0, needed, first, true, true);
        if (!ready)
          success = false;
      }
      for (int i = 0; i < n; ++i) {
        double t = at + i / rate;
        if (!ready || t < start || t >= end)
          continue;
        double sourceAt =
            (region->getStartInAudioModificationTime() + (t - start) * ratio) *
                source.rate -
            first;
        int j = int(sourceAt);
        float f = float(sourceAt - j);
        if (j < 1 || j + 2 >= source.pcm.getNumSamples())
          continue;
        auto channelSample = [&](int src) {
          if (source.sinc.empty()) {
            float a = source.pcm.getSample(src, j - 1),
                  b = source.pcm.getSample(src, j),
                  c = source.pcm.getSample(src, j + 1),
                  d = source.pcm.getSample(src, j + 2);
            return a * (-f * (f - 1) * (f - 2) / 6) +
                   b * ((f + 1) * (f - 1) * (f - 2) / 2) +
                   c * (-(f + 1) * f * (f - 2) / 2) +
                   d * ((f + 1) * f * (f - 1) / 6);
          }
          int phase = juce::jlimit(0, 1023, int(f * 1024));
          double sum = 0;
          for (int k = 0; k < 64; ++k) {
            int at = j + k - 31;
            if (at >= 0 && at < needed)
              sum += source.pcm.getSample(src, at) *
                     source.sinc[size_t(phase)][size_t(k)];
          }
          return float(sum);
        };
        for (int c = 0; c < channels; ++c) {
          float x = channelSample(std::min(c, source.pcm.getNumChannels() - 1));
          if (channels == 1 && source.pcm.getNumChannels() == 2)
            x = .5f * (channelSample(0) + channelSample(1));
          voice.audio.setSample(c, i, x);
        }
      }
      juce::dsp::AudioBlock<float> block(voice.audio);
      voice.chain->process(block.getSubBlock(0, size_t(n)), values, bpm,
                           hostBypass.load());
      for (int c = 0; c < channels; ++c)
        b.addFrom(c, offset, voice.audio, c, 0, n);
    }
    controller.meters.in.store(voice.chain->meters.in.load());
    controller.meters.compression.store(voice.chain->meters.compression.load());
    controller.meters.deEss.store(voice.chain->meters.deEss.load());
    controller.meters.duck.store(voice.chain->meters.duck.load());
    controller.meters.limiting.store(voice.chain->meters.limiting.load());
  }
  float peak = 0;
  for (int c = 0; c < b.getNumChannels(); ++c)
    peak = std::max(peak, b.getMagnitude(c, 0, b.getNumSamples()));
  controller.meters.out.store(peak);
  return success;
}
} // namespace fresh
const ARA::ARAFactory *JUCE_CALLTYPE createARAFactory() {
  return juce::ARADocumentControllerSpecialisation::createARAFactory<
      fresh::Controller>();
}
