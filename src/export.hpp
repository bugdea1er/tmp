#ifndef TMP_EXPORT_H
#define TMP_EXPORT_H

#if defined _WIN32
#if defined TMP_BUILDING_DLL
#define exported __declspec(dllexport)
#else
#define exported
#endif
#else
#define exported __attribute__((visibility("default")))
#endif

namespace tmp {
class exported directory;
class exported file;
}    // namespace tmp

#endif    // TMP_EXPORT_H
