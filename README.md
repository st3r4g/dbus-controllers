# dbus-controllers
*non-systemd controllers for dbus-broker*

## dbus-controller-dummy
dummy controller for dbus-broker

Provides a way lo launch [dbus-broker] without the requirement of any specific
init system running. This means that dbus activation is disabled and service
dependencies must be taken care of explicitly in other ways.

Build the broker with `-Dlauncher=false`.

Controller+broker are a replacement for the original dbus-daemon
implementation, and are capable of running both system and session buses.

This is also a starting point to explore the feasibility of a runit/s6
controller.

## dbus-controller-s6

Build with `meson -Ds6=enabled build`.

Usage: `dbus-controller-s6 -h`.

The [s6] controller looks in a s6 scandir for services containing the file
`data/dbus-activatable-name` with a dbus activatable name provided by the
service. It issues `s6-svc -u <service>` when it receives an activation
request.

Such dbus activatable services can be autogenerated by `tools/s6-generator`.
It is typically invoked as:
```
./s6-generator /usr/share/dbus-1/system-services/*
./s6-generator /usr/share/dbus-1/services/*
```
for the system and session bus, respectively.

You are free to further tweak this scandir as you like, for example removing
unwanted activatable services.

[dbus-broker]: https://github.com/bus1/dbus-broker
[s6]: https://skarnet.org/software/s6
