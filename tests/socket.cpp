#include <tmp/socket>

#include "utils.hpp"

#include <gtest/gtest.h>

#include <sys/socket.h>
#include <sys/un.h>

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Tests socket creation with label
TEST(socket, create_with_label) {
  socket tmpsocket = socket(std::string_view(LABEL));
  fs::path parent = tmpsocket.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpsocket));
  EXPECT_TRUE(fs::is_socket(tmpsocket));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / LABEL));
  EXPECT_TRUE(native_handle_is_valid(tmpsocket.native_handle()));

  // FIXME: what should it be?
  // fs::perms permissions = fs::status(tmpsocket).permissions();
  // EXPECT_EQ(permissions, fs::perms::owner_read | fs::perms::owner_write);
}

/// Tests socket creation without label
TEST(socket, create_without_label) {
  socket tmpsocket = socket();
  fs::path parent = tmpsocket.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpsocket));
  EXPECT_TRUE(fs::is_socket(tmpsocket));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
  EXPECT_TRUE(native_handle_is_valid(tmpsocket.native_handle()));
}

/// Tests socket creation with path
TEST(socket, create_with_path) {
  socket tmpsocket = socket(fs::current_path() / "socket");
  fs::path parent = tmpsocket.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpsocket));
  EXPECT_TRUE(fs::is_socket(tmpsocket));
  EXPECT_TRUE(fs::equivalent(parent, fs::current_path()));
  EXPECT_TRUE(native_handle_is_valid(tmpsocket.native_handle()));
}

/// Tests socket listening
TEST(socket, listening) {
  socket tmpsocket = socket();
  tmpsocket.listen([](std::string_view input) noexcept {
    EXPECT_EQ(input, std::string_view("Hello, world!"));
    return std::string("Bye, world!");
  });

  sockaddr_un address;
  address.sun_len = sizeof(address);
  address.sun_family = AF_UNIX;
  memcpy(address.sun_path, tmpsocket.path().c_str(),
         tmpsocket.path().string().length() + 1);

  int client = ::socket(PF_LOCAL, SOCK_STREAM, 0);
  connect(client, reinterpret_cast<sockaddr*>(&address), sizeof(address));

  std::string_view input = "Hello, world!";
  write(client, input.data(), input.size());

  std::string output;
  output.resize(strlen("Bye, world!"));
  ssize_t bytes_read = read(client, output.data(), INT_MAX);

  EXPECT_EQ(bytes_read, output.size());
  EXPECT_EQ(output, "Bye, world!");
}

/// Tests that destructor removes a socket
TEST(socket, destructor) {
  fs::path path;
  entry::native_handle_type handle;
  {
    socket tmpsocket = socket();
    path = tmpsocket;
    handle = tmpsocket.native_handle();
  }

  EXPECT_FALSE(fs::exists(path));
  EXPECT_FALSE(native_handle_is_valid(handle));
}

/// Tests socket move constructor
TEST(socket, move_constructor) {
  socket fst = socket();
  socket snd = socket(std::move(fst));

  fst.~socket();    // NOLINT(*-use-after-move)

  EXPECT_FALSE(snd.path().empty());
  EXPECT_TRUE(fs::exists(snd));
  EXPECT_TRUE(native_handle_is_valid(snd.native_handle()));
}

/// Tests socket move assignment operator
TEST(socket, move_assignment) {
  socket fst = socket();
  {
    socket snd = socket();

    fs::path path1 = fst;
    fs::path path2 = snd;

    entry::native_handle_type fst_handle = fst.native_handle();
    entry::native_handle_type snd_handle = snd.native_handle();

    fst = std::move(snd);

    EXPECT_FALSE(fs::exists(path1));
    EXPECT_TRUE(fs::exists(path2));

    EXPECT_TRUE(fs::exists(fst));
    EXPECT_TRUE(fs::equivalent(fst, path2));

    EXPECT_FALSE(native_handle_is_valid(fst_handle));
    EXPECT_TRUE(native_handle_is_valid(snd_handle));
  }

  EXPECT_FALSE(fst.path().empty());
  EXPECT_TRUE(native_handle_is_valid(fst.native_handle()));
}

/// Tests socket swapping
TEST(socket, swap) {
  socket fst = socket();
  socket snd = socket();

  fs::path fst_path = fst.path();
  fs::path snd_path = snd.path();
  entry::native_handle_type fst_handle = fst.native_handle();
  entry::native_handle_type snd_handle = snd.native_handle();

  std::swap(fst, snd);

  EXPECT_EQ(fst.path(), snd_path);
  EXPECT_EQ(snd.path(), fst_path);
  EXPECT_EQ(fst.native_handle(), snd_handle);
  EXPECT_EQ(snd.native_handle(), fst_handle);
}

/// Tests socket hashing
TEST(socket, hash) {
  socket tmpsocket = socket();
  std::hash hash = std::hash<socket>();

  EXPECT_EQ(hash(tmpsocket), fs::hash_value(tmpsocket.path()));
}

/// Tests socket relational operators
TEST(socket, relational) {
  socket tmpsocket = socket();

  EXPECT_TRUE(tmpsocket == tmpsocket);
  EXPECT_FALSE(tmpsocket < tmpsocket);
}
}    // namespace
}    // namespace tmp
