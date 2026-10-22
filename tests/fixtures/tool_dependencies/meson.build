project('resource-tool-boundary-fixture', 'cpp', meson_version: '>=1.7.2')
core = static_library('resource_core', 'nandina/resource/resource.cpp')
links = [core]
if get_option('injection') != 'none'
    ui = static_library('nandina', 'nandina/scene/forbidden.cpp')
    if get_option('injection') == 'direct'
        links += ui
    else
        bridge = static_library('resource_build', 'tools/resource_manifest.cpp', link_with: ui)
        links += bridge
    endif
endif
configure_file(input: 'link.rsp.in', output: 'link.rsp', copy: true)
executable('nanres', 'tools/nanres_main.cpp', link_with: links,
           link_args: ['-Wl,@' + meson.current_build_dir() / 'link.rsp'])
