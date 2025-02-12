#include <filesystem>
#include <iostream>

int main() {
  std::cout << std::filesystem::current_path() << std::endl;
  return 0;
}
