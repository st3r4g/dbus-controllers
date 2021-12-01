#include "dbus.h"
#include "policy.h"
#include "sd-bus.h"
#include "util.h"

#include <assert.h>
#include <libgen.h>

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

static void parse_set_activation_enviroment(sd_bus_message *m) {
	int r = sd_bus_message_enter_container(m, 'a', "{ss}");
	assert(r == 1);

	while (!sd_bus_message_at_end(m, 0)) {
		const char *key, *value;

		r = sd_bus_message_read(m, "{ss}", &key, &value);
		assert(r >= 0);

		fprintf(stderr, "Warning: ignoring SetActivationEnvironment %s=%s\n", key, value);
	}

	r = sd_bus_message_exit_container(m);
	assert(r == 1);
}

static int on_message(sd_bus_message *m, void *userdata, sd_bus_error *error) {
	sd_bus* bus_controller = userdata;

	const char* object_path = sd_bus_message_get_path(m);
	if (!sd_bus_message_is_signal(m, "org.bus1.DBus.Name", "Activate")) {
                assert(sd_bus_message_is_signal(m, "org.bus1.DBus.Broker", "SetActivationEnvironment"));
		parse_set_activation_enviroment(m);
		return 0;
	}

	uint64_t serial;
	int r = sd_bus_message_read(m, "t", &serial);
#ifdef HAVE_S6
	char* path = strdup(object_path);
	r = start_service(atoi(basename(path)));
	free(path);
#endif
	if (r != 0) {
		r = sd_bus_call_method(bus_controller, NULL, object_path, "org.bus1.DBus.Name", "Reset", NULL, NULL, "t", serial);
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

static sd_bus* bus_controller;

void controller_setup(int fd, const char* dbus_socket_path) {
	int r;
	r = sd_bus_new(&bus_controller);
	if (r < 0) handle_error("sd_bus_new");
	r = sd_bus_set_fd(bus_controller, fd, fd);
	if (r < 0) handle_error("sd_bus_set_fd");
	r = sd_bus_add_object_vtable(bus_controller, NULL, "/org/bus1/DBus/Controller", "org.bus1.DBus.Controller", launcher_vtable, NULL);
	if (r < 0) handle_error("sd_bus_add_object_vtable");
	sd_bus_add_filter(bus_controller, NULL, on_message, bus_controller);
	if (r < 0) handle_error("sd_bus_add_filter");
	r = sd_bus_start(bus_controller);
	if (r < 0) handle_error("sd_bus_start");
	if (launcher_add_listener(bus_controller, dbus_create_socket(dbus_socket_path)))
		printf("AddListener failed\n");

#ifdef HAVE_S6
        add_service(bus_controller);
#endif
}

void controller_run() {
        for (;;) {
                /* Process requests */
                int r = sd_bus_process(bus_controller, NULL);
                if (r < 0) handle_error("Failed to process bus");
                if (r > 0) /* we processed a request, try to process another one, right-away */
                        continue;

                /* Wait for the next request to process */
                r = sd_bus_wait(bus_controller, (uint64_t) -1);
                if (r < 0) handle_error("Failed to wait on bus");
        }
}

#ifdef HAVE_S6

#include <skalibs/iopause.h>
#include <skalibs/selfpipe.h>

void controller_run_signals() {
	iopause_fd x[2] = {
		{ sd_bus_get_fd(bus_controller), IOPAUSE_READ, 0 },
		{ selfpipe_init(), IOPAUSE_READ, 0 },
	};
	int r;
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGTERM);
	sigaddset(&set, SIGINT);
	r = selfpipe_trapset(&set);
	for (;;) {
		/* Wait for the next request to process */
		r = iopause(x, 2, NULL, NULL);
		if (r < 0) handle_error("Failed to wait on bus");
		if (x[0].revents & IOPAUSE_READ)
			/* Process requests */
			while ((r = sd_bus_process(bus_controller, NULL)))
				if (r < 0) handle_error("Failed to process bus");
		if (x[1].revents & IOPAUSE_READ) {
			int c = selfpipe_read();
			fprintf(stderr, "caught signal %d, stopping...\n", c);
			break;
		}
	}
	selfpipe_finish();
	stop_all_services();
}

#endif
