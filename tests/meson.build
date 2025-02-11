gtest_main_dep = dependency('gtest_main', main: true, fallback: ['gtest', 'gtest_main_dep'])

unit_test = executable(
  meson.project_name() + '.test',
  'directory.cpp',
  'file.cpp',
  dependencies: [gtest_main_dep, tmp_dep],
  cpp_args: [
    '-DLABEL="com.github.bugdea1er.tmp"',
    '-DBUILD_DIR="' + meson.current_build_dir() + '"',
  ],
  build_by_default: false,
)

test('tmp unit tests', unit_test, args: ['--gtest_color=yes'])
