#include <tmp/entry>
#include <tmp/socket>

#include "utils.hpp"

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

#ifndef _WIN32
#include <cerrno>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace tmp {
namespace {

/// Creates a temporary unix domain socket at the given path
/// @note The given path will be overwritten
/// @param path     A path for the temporary socket
/// @returns A path to the created temporary socket and a handle to it
/// @throws std::filesystem::filesystem_error if cannot create a socket
std::pair<fs::path, entry::native_handle_type> create_socket(fs::path path) {
  if (fs::exists(path)) {
    fs::remove_all(path);
  }

  std::string_view path_string = path.native();
  if (path_string.length() >= sizeof(sockaddr_un::sun_path)) {
    std::error_code ec = std::make_error_code(std::errc::filename_too_long);
    throw fs::filesystem_error("Cannot create a temporary socket", ec);
  }

  entry::native_handle_type handle = ::socket(PF_LOCAL, SOCK_STREAM, 0);
  // FIXME: check if handle is valid

  sockaddr_un address = sockaddr_un();
  address.sun_family = AF_UNIX;
  std::copy(path_string.begin(), path_string.end(), address.sun_path);

  int ret = bind(handle, (sockaddr*)&address, sizeof(address));
  if (ret != 0) {
    std::error_code ec = std::error_code(errno, std::system_category());
    throw fs::filesystem_error("Cannot create a temporary socket", ec);
  }

  return std::pair(std::move(path), handle);
}

/// Creates a temporary unix domain socket with the given prefix in the system's
/// temporary directory
/// @param label     A label to attach to the temporary socket path
/// @returns A path to the created temporary socket and a handle to it
/// @throws fs::filesystem_error  if cannot create a temporary socket
/// @throws std::invalid_argument if the label is ill-formatted
std::pair<fs::path, entry::native_handle_type>
create_socket(std::string_view label) {
  fs::path::string_type path = make_pattern(label, "");

  std::error_code ec;
  create_parent(path, ec);
  if (ec) {
    throw fs::filesystem_error("Cannot create a temporary socket", ec);
  }

  mktemp(path.data());    // FIXME: check for errors
  return create_socket(fs::path(path));
}
}    // namespace

socket::socket(std::string_view label)
    : entry(create_socket(label)) {}

socket::socket(fs::path path)
    : entry(create_socket(std::move(path))) {}

void socket::listen(acceptor function) {
  native_handle_type handle = native_handle();

  int ret = ::listen(handle, SOMAXCONN);
  if (ret != 0) {
    std::error_code ec = std::error_code(errno, std::system_category());
    throw fs::filesystem_error("Cannot listen a temporary socket", ec);
  }

  std::thread thread = std::thread([handle, function] {
    while (true) {
      int client = accept(handle, nullptr, nullptr);
      if (client < 0) {
        if (errno == EBADF) {    // closed via `~socket()`
          break;
        }

        continue;
      }

      std::string input;
      input.resize(1024);    // FIXME: magic constant

      ssize_t received = read(client, input.data(), input.capacity());
      if (received <= 0) {
        close(client);
        continue;
      }

      input.resize(received);

      std::string response = function(input);

      write(client, response.c_str(), response.size());
      close(client);
    }
  });

  thread.detach();
}

socket::~socket() noexcept = default;    // FIXME: shutdown(fd, SHUT_RD)?

socket::socket(socket&&) noexcept = default;
socket& socket::operator=(socket&& other) noexcept = default;
}    // namespace tmp

std::size_t
std::hash<tmp::socket>::operator()(const tmp::socket& socket) const noexcept {
  return std::hash<tmp::entry>()(socket);
}
