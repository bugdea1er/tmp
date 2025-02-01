project('tmp', 'cpp', version: '2.0', meson_version: '>=1.3')

include_directory = include_directories('include')
cxx = meson.get_compiler('cpp')

std_fs_dep = cxx.find_library('stdc++fs', required: false)

tmp = library(
  'tmp',
  'src/create.cpp',
  'src/entry.cpp',
  'src/file.cpp',
  'src/filebuf.cpp',
  'src/directory.cpp',
  'src/move.cpp',
  install: true,
  include_directories: include_directory,
  gnu_symbol_visibility: 'hidden',
  version: meson.project_version(),
  dependencies: [std_fs_dep],
  cpp_shared_args: ['-DTMP_BUILDING_DLL'],
)

tmp_dep = declare_dependency(
  link_with: tmp,
  include_directories: include_directory,
  dependencies: [std_fs_dep],
  version: meson.project_version(),
)

install_headers(
  'include/tmp/directory',
  'include/tmp/entry',
  'include/tmp/export',
  'include/tmp/file',
  subdir: 'tmp',
)

if not meson.is_subproject()
  if get_option('build_tests')
    subdir('tests')
  endif
  pkg = import('pkgconfig')
  pkg.generate(
    libraries: tmp,
    version: meson.project_version(),
    name: meson.project_name(),
    filebase: 'lib' + meson.project_name(),
    description: 'RAII-wrappers for unique temporary files and directories for modern C++',
    url: 'https://github.com/bugdea1er/tmp',
  )
endif
