system_sources = [
  'EventPipe.cxx',
]

if host_machine.system() == 'linux'
  system_sources += [
    'KernelVersion.cxx',
    'EventFD.cxx',
    'SignalFD.cxx',
    'EpollFD.cxx',
  ]
endif

if host_machine.system() == 'windows'
  system_sources += [
    'windows/Clock.cxx',
  ]
endif

system = static_library(
  'system',
  system_sources,
  include_directories: inc,
  dependencies: [
    fmt_dep,
  ],
)

if is_windows
  winsock_dep = c_compiler.find_library('ws2_32')
else
  winsock_dep = dependency('', required: false)
endif

system_dep = declare_dependency(
  link_with: system,
  dependencies: [
    winsock_dep,
  ],
)
