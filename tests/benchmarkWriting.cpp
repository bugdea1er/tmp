#include <tmp/file>

#include <benchmark/benchmark.h>
#include <sstream>
#include <string>

std::string generateRandomString(int length) {
  const std::string characters =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  std::string randomString;

  for (int i = 0; i < length; ++i) {
    int index = rand() % characters.size();
    randomString += characters[index];
  }

  return randomString;
}

static void WritingToStringbuf(benchmark::State& state) {
  std::string buffer = generateRandomString(1024L);
  std::stringstream stream;
  for (auto _ : state) {
    for (std::size_t i = 0; i < 1024 * 1024L; i++) {
      stream << buffer;
    }
  }
}

static void WritingToTmpFile(benchmark::State& state) {
  std::string buffer = generateRandomString(1024L);
  tmp::file file;
  for (auto _ : state) {
    for (std::size_t i = 0; i < 1024 * 1024L; i++) {
      file << buffer;
    }
  }
}

BENCHMARK(WritingToStringbuf);
BENCHMARK(WritingToTmpFile);

BENCHMARK_MAIN();
