# dbus-controller-dummy
dummy controller for dbus-broker

Provides a way lo launch [dbus-broker] without the requirement of any specific
init system running. This means that dbus activation is disabled and service
dependencies must be taken care of explicitly in other ways.

This is also a starting point to explore the feasibility of a runit/s6
controller.

[dbus-broker]: https://github.com/bus1/dbus-broker
