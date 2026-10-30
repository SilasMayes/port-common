project('port-common', 'c',
    version: '0.1',
    meson_version: '>=1.12.0',
    default_options: [
        'buildtype=plain',
        'warning_level=0',
    ]
)

# Game-agnostic port layer shared by the pokeplatinum and pokeheartgold ports. Built as a
# subproject of a game, which implements include/port/port_game.h (see README.md).
# port-common sees only its own headers and libntr's: a game header included here fails
# to compile, which keeps it game-agnostic.

build_target = get_option('build_target')
if build_target == 'arm'
    error('port-common is for the port builds only (not the DS build)')
endif

# Headers, defines and the generated fx_const.h of libntr, without its libraries and crt0
# (the game links those once)
nitrosdk_headers_dep = dependency('NitroSDK').partial_dependency(
    compile_args: true,
    includes: true,
    sources: true,
)

port_common_includes = include_directories('include')
is_native = (build_target == 'linux' or build_target == 'win64')

port_common_lib = static_library('port_common',
    files(
        'src/autosave.c',
        'src/cheats.c',
        'src/save_slots.c',
        'src/save_sync.c',
        'src/save_version.c',
        'src/shiny_hunt.c',
    ),
    c_args: ['-std=c99'],
    include_directories: port_common_includes,
    dependencies: nitrosdk_headers_dep,
    # Same machine as libntr and the game's main (the desktop builds are native)
    native: is_native,
)

port_common_dep = declare_dependency(
    include_directories: port_common_includes,
    link_with: port_common_lib,
)
meson.override_dependency('port_common', port_common_dep, native: is_native)
