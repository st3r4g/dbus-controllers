#include "dbus.h"
#include "policy.h"
#include "sd-bus.h"
#include "util.h"

static int launcher_add_listener(sd_bus* bus_controller, int fd_listen) {
	sd_bus_message *m = NULL;
	int r;

	r = sd_bus_message_new_method_call(bus_controller, &m, NULL,
		"/org/bus1/DBus/Broker", "org.bus1.DBus.Broker", "AddListener");
	if (r < 0)
		return 1;

	r = sd_bus_message_append(m, "oh", "/org/bus1/DBus/Listener/0", fd_listen);
	if (r < 0)
		return 1;

	policy(m);

	sd_bus_error error = SD_BUS_ERROR_NULL;
	r = sd_bus_call(bus_controller, m, 0, &error, NULL);
	if (r < 0) {
		printf("sd_bus_call falied: %s %s\n", error.name, error.message);
		return 1;
	}

	return 0;
}

static int bus_method_reload_config(sd_bus_message *message, void *userdata, sd_bus_error *error) {
	/*
         * Do nothing...
         */
	return sd_bus_reply_method_return(message, NULL);
}

static const sd_bus_vtable launcher_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("ReloadConfig", NULL, NULL, bus_method_reload_config, 0),
	SD_BUS_VTABLE_END
};

void controller_setup(int fd, const char* dbus_socket_path) {
	sd_bus* bus_controller;
	int r;
	r = sd_bus_new(&bus_controller);
	if (r < 0) handle_error("sd_bus_new");
	r = sd_bus_set_fd(bus_controller, fd, fd);
	if (r < 0) handle_error("sd_bus_set_fd");
	r = sd_bus_add_object_vtable(bus_controller, NULL, "/org/bus1/DBus/Controller", "org.bus1.DBus.Controller", launcher_vtable, NULL);
	if (r < 0) handle_error("sd_bus_add_object_vtable");
	//sd_bus_add_filter(bus_controller, NULL, launcher_on_message, NULL);
	r = sd_bus_start(bus_controller);
	if (r < 0) handle_error("sd_bus_start");
	if (launcher_add_listener(bus_controller, dbus_create_socket(dbus_socket_path)))
		printf("AddListener failed\n");
}
