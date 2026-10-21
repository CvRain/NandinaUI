project('nandina_sdk_app_contract', 'cpp', default_options: ['cpp_std=c++26'])
nandina = subproject('nandina', default_options: [
    'build_tests=false',
    'physics2d=disabled',
    'sdl_backend=enabled',
    'sdl3_source=vendored',
])
nandina_dep = nandina.get_variable('nandina_dep')
executable('probe', 'main.cpp', dependencies: nandina_dep)
