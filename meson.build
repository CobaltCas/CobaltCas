project('CobaltCas', 'cpp', version: '1.0.0', default_options: ['cpp_std=c++17'])
add_project_arguments('-Wunused-variable', language: 'cpp')

shared_library(
    'pcsclite',
    files(
        'src/card.cpp',
        'src/crypto.cpp',
        'src/key_manager.cpp',
        'src/log.cpp',
        'src/utils.cpp',
        'src/winscard.cpp',
    ),
    dependencies: [dependency('libpcsclite')],
    install: true,
    install_dir: '/usr/lib/@0@-linux-gnu/cobaltcas/'.format(host_machine.cpu_family()),
    version: '1.0.0',
)
