#ifndef TMP_EXPORT_H
#define TMP_EXPORT_H

#include <filesystem>

#if defined _WIN32
#if defined TMP_BUILDING_DLL
#define TMP_EXPORT __declspec(dllexport)
#else
#define TMP_EXPORT
#endif
#else
#define TMP_EXPORT __attribute__((visibility("default")))
#endif

namespace tmp {
class TMP_EXPORT directory;
}    // namespace tmp

#endif    // TMP_EXPORT_H
