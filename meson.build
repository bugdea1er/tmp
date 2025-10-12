project(
  'tmp',
  'cpp',
  version: '4',
  default_options: ['cpp_std=c++17'],
)

include_directory = include_directories('include')

filesystem_proj = subproject('filesystem')
filesystem_dep = filesystem_proj.get_variable('filesystem_dep')

tmp = library(
  'tmp',
  'src/file.cpp',
  'src/directory.cpp',
  install: true,
  include_directories: include_directory,
  gnu_symbol_visibility: 'default',
  version: meson.project_version(),
  dependencies: [filesystem_dep],
)

tmp_dep = declare_dependency(
  link_with: tmp,
  include_directories: include_directory,
  dependencies: [filesystem_dep],
  version: meson.project_version(),
)

install_headers(
  'include/tmp/directory',
  'include/tmp/file',
  subdir: 'tmp',
)

if not meson.is_subproject()
  if get_option('build_tests')
    subdir('tests')
  endif
  pkg = import('pkgconfig')
  pkg.generate(
    tmp,
    libraries: [filesystem_dep],
    version: meson.project_version(),
    filebase: 'lib' + meson.project_name(),
    description: 'RAII-wrappers for unique temporary files and directories for modern C++',
    url: 'https://github.com/bugdea1er/tmp',
  )
endif
