#ifndef TMP_TESTS_UTILS_H
#define TMP_TESTS_UTILS_H

#include <tmp/entry>

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#endif

namespace tmp {

/// Checks if the given file handle is valid
/// @param handle handle to check
/// @returns whether the handle is valid
inline bool native_handle_is_valid(entry::native_handle_type handle) {
#ifdef _WIN32
  BY_HANDLE_FILE_INFORMATION info;
  return GetFileInformationByHandle(handle, &info);
#else
  return fcntl(handle, F_GETFD) != -1; 
#endif
}
}    // namespace tmp

#endif    // TMP_TESTS_UTILS_H
