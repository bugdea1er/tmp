#include <tmp/directory>
#include <tmp/file>

#include "checks.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace tmp {
namespace {

namespace fs = std::filesystem;

/// Tests file creation with label
TEST(file, create_with_label) {
  file tmpfile = file(LABEL);
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path() / LABEL));
  EXPECT_TRUE(native_handle_is_valid(tmpfile.native_handle()));

  fs::perms permissions = fs::status(tmpfile).permissions();
#ifdef _WIN32
  // GetTempFileNameW creates a file with all permissions
  EXPECT_EQ(permissions, fs::perms::all);
#else
  // mkstemp creates a file that can only be read and written by the owner
  EXPECT_EQ(permissions, fs::perms::owner_read | fs::perms::owner_write);
#endif
}

/// Tests file creation without label
TEST(file, create_without_label) {
  file tmpfile = file();
  fs::path parent = tmpfile.path().parent_path();

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_TRUE(fs::equivalent(parent, fs::temp_directory_path()));
  EXPECT_TRUE(native_handle_is_valid(tmpfile.native_handle()));
}

/// Tests file creation with extension
TEST(file, create_with_extension) {
  file tmpfile = file("", ".test");

  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::is_regular_file(tmpfile));
  EXPECT_EQ(tmpfile.path().extension(), ".test");
  EXPECT_TRUE(native_handle_is_valid(tmpfile.native_handle()));
}

/// Tests multiple file creation with the same label
TEST(file, create_multiple) {
  file fst = file(LABEL);
  file snd = file(LABEL);

  EXPECT_FALSE(fs::equivalent(fst, snd));
}

/// Tests error handling with invalid labels
TEST(file, create_invalid_label) {
  EXPECT_THROW(file("multi/segment"), std::invalid_argument);
  EXPECT_THROW(file("/root"), std::invalid_argument);
  EXPECT_THROW(file(".."), std::invalid_argument);
  EXPECT_THROW(file("."), std::invalid_argument);

  fs::path root = fs::temp_directory_path().root_name();
  if (!root.empty()) {
    EXPECT_THROW(file(root.string() + "relative"), std::invalid_argument);
    EXPECT_THROW(file(root.string() + "/root"), std::invalid_argument);
  }
}

/// Tests error handling with invalid extensions
TEST(file, create_invalid_extension) {
  EXPECT_THROW(file("", "multi/segment"), std::invalid_argument);
  EXPECT_THROW(file("", "/root"), std::invalid_argument);
  EXPECT_THROW(file("", "/.."), std::invalid_argument);
  EXPECT_THROW(file("", "/."), std::invalid_argument);
}

/// Tests creation of a temporary copy of a file
TEST(file, copy_file) {
  file tmpfile = file();
  tmpfile.write("Hello, world!");

  file copy = file::copy(tmpfile);
  EXPECT_TRUE(fs::exists(tmpfile));
  EXPECT_TRUE(fs::exists(copy));
  EXPECT_FALSE(fs::equivalent(tmpfile, copy));

  EXPECT_TRUE(fs::is_regular_file(tmpfile));

  EXPECT_EQ(copy.read(), "Hello, world!");
}

/// Tests creation of a temporary copy of a directory
TEST(file, copy_directory) {
  directory tmpdir = directory();
  EXPECT_THROW(file::copy(tmpdir), fs::filesystem_error);
}

/// Tests binary file reading
TEST(file, read_binary) {
  file tmpfile = file();
  std::ofstream stream = std::ofstream(tmpfile.path(), std::ios::binary);

  stream << "Hello,\nworld!\n" << std::flush;

  EXPECT_EQ(tmpfile.read(), "Hello,\nworld!\n");
}

/// Tests text file reading
TEST(file, read_text) {
  file tmpfile = file::text();
  std::ofstream stream = std::ofstream(tmpfile.path());

  stream << "Hello,\nworld!\n" << std::flush;

  EXPECT_EQ(tmpfile.read(), "Hello,\nworld!\n");
}

/// Tests file reading error reporting
TEST(file, read_error) {
  file tmpfile = file();

#ifdef _WIN32
  CloseHandle(tmpfile.native_handle());
#else
  ::close(tmpfile.native_handle());
#endif

  EXPECT_THROW(tmpfile.read(), fs::filesystem_error);
}

/// Tests binary file writing
TEST(file, write_binary) {
  file tmpfile = file();
  tmpfile.write("Hello\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello\n");
  }

  tmpfile.write("world!\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "world!\n");
  }
}

/// Tests text file writing
TEST(file, write_text) {
  file tmpfile = file::text();
  tmpfile.write("Hello\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello\r\n");
#else
    EXPECT_EQ(content, "Hello\n");
#endif
  }

  tmpfile.write("world!\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "world!\r\n");
#else
    EXPECT_EQ(content, "world!\n");
#endif
  }
}

/// Tests file writing error reporting
TEST(file, write_error) {
  file tmpfile = file();
#ifdef _WIN32
  CloseHandle(tmpfile.native_handle());
#else
  ::close(tmpfile.native_handle());
#endif

  EXPECT_THROW(tmpfile.write("Hello!"), fs::filesystem_error);
}

/// Tests binary file appending
TEST(file, append_binary) {
  file tmpfile = file();
  std::ofstream(tmpfile.path(), std::ios::binary) << "Hello, ";

  tmpfile.append("world");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello, world");
  }

  tmpfile.append("!");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello, world!");
  }
}

/// Tests text file appending
TEST(file, append_text) {
  file tmpfile = file::text();
  std::ofstream(tmpfile.path()) << "Hello,";

  tmpfile.append("\n world");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello,\r\n world");
#else
    EXPECT_EQ(content, "Hello,\n world");
#endif
  }

  tmpfile.append("!\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello,\r\n world!\r\n");
#else
    EXPECT_EQ(content, "Hello,\n world!\n");
#endif
  }
}

/// Tests file appending error reporting
TEST(file, append_error) {
  file tmpfile = file();

#ifdef _WIN32
  CloseHandle(tmpfile.native_handle());
#else
  ::close(tmpfile.native_handle());
#endif

  EXPECT_THROW(tmpfile.append("world!"), fs::filesystem_error);
}

/// Tests binary file reading from input_stream
TEST(file, input_stream_binary) {
  file tmpfile = file();
  std::ofstream ostream = std::ofstream(tmpfile.path(), std::ios::binary);

  ostream << "Hello,\nworld!\n" << std::flush;

  auto istream = tmpfile.input_stream();
  auto content = std::string(std::istreambuf_iterator<char>(istream), {});

  EXPECT_EQ(content, "Hello,\nworld!\n");
}

/// Tests text file reading from input_stream
TEST(file, input_stream_text) {
  file tmpfile = file::text();
  std::ofstream ostream = std::ofstream(tmpfile.path());

  ostream << "Hello,\nworld!\n" << std::flush;

  auto istream = tmpfile.input_stream();
  auto content = std::string(std::istreambuf_iterator<char>(istream), {});

  EXPECT_EQ(content, "Hello,\nworld!\n");
}

/// Tests binary file writing to output_stream
TEST(file, output_stream_write_binary) {
  file tmpfile = file();
  std::ofstream ostream = tmpfile.output_stream();
  ostream << "Hello\n" << std::flush;

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello\n");
  }

  tmpfile.write("world!\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "world!\n");
  }
}

/// Tests text file writing to output_stream
TEST(file, output_stream_write_text) {
  file tmpfile = file::text();
  std::ofstream ostream = tmpfile.output_stream();
  ostream << "Hello\n" << std::flush;

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello\r\n");
#else
    EXPECT_EQ(content, "Hello\n");
#endif
  }

  tmpfile.write("world!\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "world!\r\n");
#else
    EXPECT_EQ(content, "world!\n");
#endif
  }
}

/// Tests text file writing to output_stream even when binary was requested
TEST(file, output_stream_write_text_binary) {
  file tmpfile = file::text();
  std::ofstream ostream = tmpfile.output_stream(std::ios::binary);
  ostream << "Hello\n" << std::flush;

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello\r\n");
#else
    EXPECT_EQ(content, "Hello\n");
#endif
  }

  tmpfile.write("world!\n");

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "world!\r\n");
#else
    EXPECT_EQ(content, "world!\n");
#endif
  }
}

/// Tests binary file appending to output_stream
TEST(file, output_stream_append_binary) {
  file tmpfile = file();
  std::ofstream(tmpfile.path(), std::ios::binary) << "Hello,\n ";

  {
    std::ofstream ostream = tmpfile.output_stream(std::ios::app);
    ostream << "world" << std::flush;
  }

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello,\n world");
  }

  {
    std::ofstream ostream = tmpfile.output_stream(std::ios::app);
    ostream << "!" << std::flush;
  }

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
    EXPECT_EQ(content, "Hello,\n world!");
  }
}

/// Tests text file appending to output_stream
TEST(file, output_stream_append_text) {
  file tmpfile = file::text();
  std::ofstream(tmpfile.path()) << "Hello,\n ";

  {
    std::ofstream ostream = tmpfile.output_stream(std::ios::app);
    ostream << "world" << std::flush;
  }

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello,\r\n world");
#else
    EXPECT_EQ(content, "Hello,\n world");
#endif
  }

  {
    std::ofstream ostream = tmpfile.output_stream(std::ios::app);
    ostream << "!" << std::flush;
  }

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello,\r\n world!");
#else
    EXPECT_EQ(content, "Hello,\n world!");
#endif
  }
}

/// Tests text file appending to output_stream even when binary was requested
TEST(file, output_stream_append_text_binary) {
  file tmpfile = file::text();
  std::ofstream(tmpfile.path()) << "Hello,\n ";

  {
    auto ostream = tmpfile.output_stream(std::ios::app | std::ios::binary);
    ostream << "world" << std::flush;
  }

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello,\r\n world");
#else
    EXPECT_EQ(content, "Hello,\n world");
#endif
  }

  {
    auto ostream = tmpfile.output_stream(std::ios::app | std::ios::binary);
    ostream << "!" << std::flush;
  }

  {
    auto stream = std::ifstream(tmpfile.path(), std::ios::binary);
    auto content = std::string(std::istreambuf_iterator<char>(stream), {});
#ifdef _WIN32
    EXPECT_EQ(content, "Hello,\r\n world!");
#else
    EXPECT_EQ(content, "Hello,\n world!");
#endif
  }
}

/// Tests that destructor removes a file
TEST(file, destructor) {
  fs::path path;
  file::native_handle_type handle;
  {
    file tmpfile = file();
    path = tmpfile;
    handle = tmpfile.native_handle();
  }

  EXPECT_FALSE(fs::exists(path));
  EXPECT_FALSE(native_handle_is_valid(handle));
}

/// Tests file move constructor
TEST(file, move_constructor) {
  file fst = file();
  file snd = file(std::move(fst));

  fst.~file();    // NOLINT(*-use-after-move)

  EXPECT_FALSE(snd.path().empty());
  EXPECT_TRUE(fs::exists(snd));
  EXPECT_TRUE(native_handle_is_valid(snd.native_handle()));
}

/// Tests file move assignment operator
TEST(file, move_assignment) {
  file fst = file();
  {
    file snd = file();

    fs::path path1 = fst;
    fs::path path2 = snd;

    file::native_handle_type fst_handle = fst.native_handle();
    file::native_handle_type snd_handle = snd.native_handle();

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

/// Tests file swapping
TEST(file, swap) {
  file fst = file();
  file snd = file();

  fs::path fst_path = fst.path();
  fs::path snd_path = snd.path();
  file::native_handle_type fst_handle = fst.native_handle();
  file::native_handle_type snd_handle = snd.native_handle();

  std::swap(fst, snd);

  EXPECT_EQ(fst.path(), snd_path);
  EXPECT_EQ(snd.path(), fst_path);
  EXPECT_EQ(fst.native_handle(), snd_handle);
  EXPECT_EQ(snd.native_handle(), fst_handle);
}

/// Tests file hashing
TEST(file, hash) {
  file tmpfile = file();
  std::hash hash = std::hash<file>();

  EXPECT_EQ(hash(tmpfile), fs::hash_value(tmpfile.path()));
}

/// Tests file relational operators
TEST(file, relational) {
  file tmpfile = file();

  EXPECT_TRUE(tmpfile == tmpfile);
  EXPECT_FALSE(tmpfile < tmpfile);
}
}    // namespace
}    // namespace tmp
