#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <chrono>
#include <span>
#include <vector>

using Clock    = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<double>;

constexpr size_t N = 5'000'000;

/// Initialize array with random 64-bit floating-point values between 0 and 1.
auto array_init(std::span<double> array) -> void {
  srand48(42);
  for (size_t i = 0; i < array.size(); ++i) {
    array[i] = drand48();
  }
}

/// Compare array values to 0.5 and set the result array accordingly.
auto array_test(std::span<double const> array, std::span<int> result) -> void {
  assert(array.size() == result.size());
  for (size_t i = 0; i < array.size(); ++i) {
    if (array[i] < 0.5) {
      result[i] = 1;
    } else {
      result[i] = 0;
    }
  }
}

auto main(void) -> int {
  // Allocate memory for 3 arrays
  std::vector<double> a_sorted(N);
  std::vector<double> a_unsorted(N);
  std::vector<int> res(N);

  // Initialize arrays
  array_init(a_unsorted);
  array_init(a_sorted);
  std::sort(a_sorted.begin(), a_sorted.end());

  // Measure execution time on the unsorted array
  auto t0 = Clock::now();
  for (size_t i = 0; i < 31; ++i) {
    array_test(a_unsorted, res);
  }
  Duration elapsed_unsorted = Clock::now() - t0;

  // Measure execution time on the sorted array
  auto t1 = Clock::now();
  for (size_t i = 0; i < 31; ++i) {
    array_test(a_sorted, res);
  }
  Duration elapsed_sorted = Clock::now() - t1;

  // Print results
  std::printf("Unsorted array: %3lf s\n", elapsed_unsorted.count());
  std::printf("Sorted array:   %3lf s\n", elapsed_sorted.count());
  std::printf("Speedup:        %3lfx faster with sorted array\n", elapsed_unsorted / elapsed_sorted);

  return 0;
}
