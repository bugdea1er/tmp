project(
  'tmp',
  'cpp',
  version: '2.0',
  default_options: ['cpp_std=c++17'],
)

include_directory = include_directories('include')

filesystem_proj = subproject('filesystem')
filesystem_dep = filesystem_proj.get_variable('filesystem_dep')

default_library = get_option('default_library')
if default_library != 'static'
  dll_usage_args = ['-DTMP_SHARED']
  if host_machine.system() == 'windows'
    dll_build_args = ['-DTMP_BUILDING_DLL']
  else
    dll_build_args = []
  endif
else
  dll_usage_args = []
  dll_build_args = []
endif

tmp = library(
  'tmp',
  'src/create.cpp',
  'src/file.cpp',
  'src/directory.cpp',
  install: true,
  include_directories: include_directory,
  gnu_symbol_visibility: 'hidden',
  version: meson.project_version(),
  dependencies: [filesystem_dep],
  cpp_args: dll_build_args,
)

tmp_dep = declare_dependency(
  link_with: tmp,
  include_directories: include_directory,
  dependencies: [filesystem_dep],
  version: meson.project_version(),
  compile_args: dll_usage_args,
)

install_headers(
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
    tmp,
    extra_cflags: dll_usage_args,
    libraries: [filesystem_dep],
    version: meson.project_version(),
    filebase: 'lib' + meson.project_name(),
    description: 'RAII-wrappers for unique temporary files and directories for modern C++',
    url: 'https://github.com/bugdea1er/tmp',
  )
endif
