#ifndef TMP_ABI_H
#define TMP_ABI_H

#if defined _WIN32
#define abi __declspec(dllexport)
#else
#define abi __attribute__((visibility("default")))
#endif

namespace tmp {
class abi directory;
class abi file;
}    // namespace tmp

#endif    // TMP_ABI_H
