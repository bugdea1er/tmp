project('filesystem', 'cpp')

filesystem_test_source = files('test.cpp')

cxx = meson.get_compiler('cpp')
std_cpp_fs = cxx.find_library('stdc++fs', required: false)
cpp_fs = cxx.find_library('c++fs', required: false)
if (cxx.links(filesystem_test_source, name: 'filesystem without a separate lib'))
  filesystem_dep = declare_dependency()
elif (
  cxx.links(
    filesystem_test_source,
    dependencies: [std_cpp_fs],
    name: 'filesystem with stdc++fs',
  )
)
  filesystem_dep = std_cpp_fs
elif (
  cxx.links(
    filesystem_test_source,
    dependencies: [cpp_fs],
    name: 'filesystem with c++fs',
  )
)
  filesystem_dep = cpp_fs
else
  error('Cannot use the standard C++ filesystem library')
endif
