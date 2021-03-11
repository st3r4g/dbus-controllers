#include "controller.h"
#include "syslog.h"
#include "util.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#define STR_INT_MAX 16

static const char* default_dbus_socket_path = "/run/dbus/system_bus_socket";
static const char* dummy_machine_id = "00000000000000000000000000000001";

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
  -s                    open a syslog connection for dbus-broker\n\
  -3                    notify readiness on fd 3\n\
  -h                    show this help message and exit\n";

int main(int argc, char* argv[]) {
	const char* dbus_socket_path = default_dbus_socket_path;
	bool notif = false;
	bool syslog = false;

	int opt;
	while ((opt = getopt(argc, argv, "d:hs3")) != -1) {
		switch (opt) {
			case 'd': dbus_socket_path = optarg; break;
			case 's': syslog = true; break;
			case '3': check_3_open(); notif = true; break;
			default:
				printf(usage, default_dbus_socket_path);
				return EXIT_FAILURE;
		}
	}

	if (argc <= optind) {
		printf(usage, default_dbus_socket_path);
		return EXIT_FAILURE;
	}

	const char* const broker_path = argv[optind];

	int controller[2];
	socketpair(PF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, controller);

	int logfd = syslog ? syslog_connect_socket() : -1;

	pid_t cpid = fork();
	if (cpid == 0) {
		close(controller[0]);
		if (notif) close(3);
		char str_controller[STR_INT_MAX];
		snprintf(str_controller, sizeof(str_controller), "%d", controller[1]);
		if (logfd < 0) {
			const char* const cmdline[] = {
				"dbus-broker",
				"--controller", str_controller,
				"--machine-id", dummy_machine_id,
			NULL};
			execvp(broker_path, (char* const*)cmdline);
		} else {
			char str_log[STR_INT_MAX];
			snprintf(str_log, sizeof(str_log), "%d", logfd);
			const char* const cmdline_log[] = {
				"dbus-broker",
				"--controller", str_controller,
				"--machine-id", dummy_machine_id,
				"--log", str_log,
			NULL};
			execvp(broker_path, (char* const*)cmdline_log);
		}
		perror("Failed to execute dbus-broker");
		_exit(EXIT_FAILURE);
		/* NOTREACHED */
	} else {
		close(controller[1]);
		if (logfd >= 0) close(logfd);

#ifdef HAVE_S6
		add_dir("/usr/share/dbus-1/system-services");
#endif

		controller_setup(controller[0], dbus_socket_path);

		if (notif) {
			write(3, "\n", 1); //dbus ready to accept connections
			close(3);
		}

		controller_run();
		int wstatus;
		waitpid(cpid, &wstatus, 0);
	}
}
