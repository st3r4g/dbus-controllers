#include "dbus.h"
#include "policy.h"
#include "syslog.h"
#include "util.h"

#include "sd-bus.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>

#define STR_INT_MAX 16

static const char* default_dbus_socket_path = "/run/dbus/system_bus_socket";
static const char* syslog_path = "/dev/log";
static const char* dummy_machine_id = "00000000000000000000000000000001";

static int launcher_add_listener(sd_bus* bus_controller, int fd_listen) {
	sd_bus_message *m = NULL;
	int r;

	r = sd_bus_message_new_method_call(bus_controller,
                                           &m,
                                           NULL,
                                           "/org/bus1/DBus/Broker",
                                           "org.bus1.DBus.Broker",
                                           "AddListener");
        if (r < 0)
                return 1;

        r = sd_bus_message_append(m, "oh",
                                  "/org/bus1/DBus/Listener/0",
                                  fd_listen);
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

const sd_bus_vtable launcher_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("ReloadConfig", NULL, NULL, bus_method_reload_config, 0),
	SD_BUS_VTABLE_END
};

static void check_3_open() {
	if (fcntl(3, F_GETFD) < 0) handle_error("readiness notification");
}

const char *usage = "usage: dbus-controller-dummy [-h] dbus-broker\n\
\n\
Dummy controller for dbus-broker\n\
\n\
positional arguments:\n\
  dbus-broker           dbus-broker to execute\n\
\n\
optional arguments:\n\
  -d                    dbus socket path (default: %s)\n\
  -l                    syslog socket path (default: %s)\n\
  -3                    notify readiness on fd 3\n\
  -h                    show this help message and exit\n";

int main(int argc, char* argv[]) {
	const char* dbus_socket_path = default_dbus_socket_path;
	bool notif = false;

	int opt;
	while ((opt = getopt(argc, argv, "hd:3")) != -1) {
		switch (opt) {
			case 'd': dbus_socket_path = optarg; break;
			case '3': check_3_open(); notif = true; break;
			default:
				printf(usage, default_dbus_socket_path, syslog_path);
				return EXIT_FAILURE;
		}
	}

	if (argc <= optind) {
		printf(usage, default_dbus_socket_path, syslog_path);
		return EXIT_FAILURE;
	}

	const char* const broker_path = argv[optind];

	int controller[2];
	socketpair(PF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, controller);

	//int logfd = syslog_connect_socket(syslog_path);

	pid_t cpid = fork();
	if (cpid == 0) {
		close(controller[0]);
		if (notif) close(3);
		char str_log[STR_INT_MAX], str_controller[STR_INT_MAX];
		snprintf(str_controller, sizeof(str_controller), "%d", controller[1]);
		//snprintf(str_log, sizeof(str_log), "%d", logfd);
		const char * const argv[] = {
			"dbus-broker",
			"--controller", str_controller,
			//"--log", str_log,
			"--machine-id", dummy_machine_id,
		NULL};
		execvp(broker_path, (char * const *)argv);
		perror("Failed to execute dbus-broker");
		_exit(EXIT_FAILURE);
		/* NOTREACHED */
	} else {
		close(controller[1]);
		//close(logfd);
		sd_bus* bus_controller;
		int r;
		r = sd_bus_new(&bus_controller);
		if (r < 0) handle_error("sd_bus_new");
		r = sd_bus_set_fd(bus_controller, controller[0], controller[0]);
		if (r < 0) handle_error("sd_bus_set_fd");
		r = sd_bus_add_object_vtable(bus_controller, NULL, "/org/bus1/DBus/Controller", "org.bus1.DBus.Controller", launcher_vtable, NULL);
		if (r < 0) handle_error("sd_bus_add_object_vtable");
		//sd_bus_add_filter(bus_controller, NULL, launcher_on_message, NULL);
		r = sd_bus_start(bus_controller);
		if (r < 0) handle_error("sd_bus_start");
		if (launcher_add_listener(bus_controller, dbus_create_socket(dbus_socket_path)))
			printf("AddListener failed\n");

		if (notif) {
			write(3, "\n", 1); //dbus ready to accept connections
			close(3);
		}

		int wstatus;
		waitpid(cpid, &wstatus, 0);
	}
}
