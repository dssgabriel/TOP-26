#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

#include <fmt/core.h>
#include <stream/stream.hpp>

constexpr size_t ONE_MIB   = 1'048'576;
constexpr size_t N_SAMPLES = 11;
constexpr size_t N_REPS    = 50;

using Clock    = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double>;

namespace utils {

auto mean(std::array<double, N_SAMPLES> const& samples) -> double {
  return std::accumulate(samples.begin(), samples.end(), 0.0) / double(N_SAMPLES);
}

auto standard_deviation(std::array<double, N_SAMPLES> const& samples, double mean) {
  double tmp = 0.0;
  std::for_each(samples.begin(), samples.end(), [&](auto e) { tmp += (e - mean) * (e - mean); });
  tmp /= double(N_SAMPLES - 1);
  return std::sqrt(tmp) * 100.0 / mean;
}

} // namespace utils

template <typename Fn, typename... Args>
auto benchmark(std::string const& name, Fn fn, Args&&... args) -> void {
  std::array<double, N_SAMPLES> epochs;

  for (size_t it = 0; it < N_SAMPLES; ++it) {
    auto start = Clock::now();
    for (size_t rep = 0; rep < N_REPS; ++rep) {
      std::invoke(fn, std::forward<Args>(args)...);
    }
    Duration elapsed = Clock::now() - start;
    epochs[it]       = elapsed.count() / static_cast<double>(N_REPS);
  }

  double mean = utils::mean(epochs);
  double sdev = utils::standard_deviation(epochs, mean);
  double min  = *std::min_element(epochs.begin(), epochs.end());
  double max  = *std::max_element(epochs.begin(), epochs.end());
  std::nth_element(epochs.begin(), epochs.begin() + epochs.size() / 2, epochs.end());
  double med = epochs[N_SAMPLES / 2];
  fmt::print("{:>16} {:10.3} {:10.2} {:10.3} {:10.3} {:10.3}\n", name, mean, sdev, min, med, max);
}

auto main(int argc, char* argv[]) -> int {
  size_t size = ONE_MIB / sizeof(double);
  if (argc != 2) {
    fmt::println(stderr, "error: {} <NB_MiB>", argv[0]);
  }
  size = static_cast<size_t>(std::atoi(argv[1])) * ONE_MIB;

  std::vector<double> a(size);
  std::vector<double> b(size);
  std::vector<double> c(size);

  stream::init(a, 1.0);
  stream::init(b, 1.0);

  fmt::println(
    "{:>16} {:>10} {:>10} {:>10} {:>10} {:>10}", "name", "mean (s)", "sdev (%)", "min (s)", "med (s)", "max (s)"
  );

  benchmark("stream::init", [&]() { stream::init(c, 0.0); });
  benchmark("stream::copy", [&]() { stream::copy(c, a); });
  benchmark("stream::add", [&]() { stream::add(a, b, c); });
  benchmark("stream::scale", [&]() { stream::mul(b, 2.0, c); });
  benchmark("stream::dot", [&]() { stream::dot(a, b); });
  benchmark("stream::triad", [&]() { stream::triad(c, 0.5, a, b); });

  return 0;
}
