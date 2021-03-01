#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

static const struct {
	short sun_family;
	char sun_path[9];
} log_addr = {
	AF_UNIX,
	"/dev/log"
};

int syslog_connect_socket() {
	int log_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (log_fd < 0)
		return -1;

	int r = connect(log_fd, (void *)&log_addr, sizeof log_addr);
	if (r < 0) {
		close(log_fd);
		perror("syslog");
	}

	return r == 0 ? log_fd : -1;
}
