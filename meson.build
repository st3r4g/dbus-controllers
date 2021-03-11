project('dbus-controller-dummy', 'c')

if get_option('sd-bus-provider') == 'auto'
assert(get_option('auto_features').auto(), 'sd-bus-provider must not be set to auto since auto_features != auto')
sdbus = dependency('libsystemd',
required: false,
not_found_message: 'libsystemd not found, trying libelogind',
)
if not sdbus.found()
sdbus = dependency('libelogind',
required: false,
not_found_message: 'libelogind not found, trying basu',
)
endif
if not sdbus.found()
sdbus = dependency('basu',
required: false,
)
endif
if not sdbus.found()
error('Neither libsystemd, nor libelogind, nor basu was found')
endif
else
sdbus = dependency(get_option('sd-bus-provider'))
endif
add_project_arguments('-DHAVE_' + sdbus.name().to_upper() + '=1', language: 'c')

executable('dbus-controller-dummy', ['controller.c', 'dbus.c', 'main.c', 'policy.c', 'syslog.c'], dependencies: [sdbus])

if get_option('s6-rc') == true
inih = dependency('inih')
tllist = dependency('tllist')

executable('dbus-controller-s6', ['activation.c', 'controller.c', 'dbus.c', 'main.c', 'policy.c', 'syslog.c'], dependencies: [inih, sdbus, tllist], c_args : '-DHAVE_S6=1')
endif
