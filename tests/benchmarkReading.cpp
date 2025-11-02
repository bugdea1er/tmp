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

static void ReadingFromStringbuf(benchmark::State& state) {
  std::string buffer = generateRandomString(1024L);
  std::stringstream stream;

  for (std::size_t i = 0; i < 1024 * 1024L; i++) {
    stream << buffer;
  }

  for (auto _ : state) {
    stream.seekg(0, std::ios::beg);
    for (std::size_t i = 0; i < 1024 * 1024L; i++) {
      stream.read(buffer.data(), buffer.size());
    }
  }
}

static void ReadingFromTmpFile(benchmark::State& state) {
  std::string buffer = generateRandomString(1024L);
  tmp::file file;

  for (std::size_t i = 0; i < 1024 * 1024L; i++) {
    file << buffer;
  }

  for (auto _ : state) {
    file.seekg(0, std::ios::beg);
    for (std::size_t i = 0; i < 1024 * 1024L; i++) {
      file.read(buffer.data(), buffer.size());
    }
  }
}

BENCHMARK(ReadingFromStringbuf);
BENCHMARK(ReadingFromTmpFile);

BENCHMARK_MAIN();
