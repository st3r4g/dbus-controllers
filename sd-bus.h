#ifndef sd_bus_h_INCLUDED
#define sd_bus_h_INCLUDED

#ifdef HAVE_LIBSYSTEMD
#include <systemd/sd-bus.h>
#elif HAVE_LIBELOGIND
#include <elogind/sd-bus.h>
#elif HAVE_BASU
#include <basu/sd-bus.h>
#endif

#endif // sd_bus_h_INCLUDED
