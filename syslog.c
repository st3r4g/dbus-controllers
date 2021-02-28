#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>

int syslog_connect_socket(const char* syslog_path) {
	struct sockaddr_un addr;
	socklen_t addr_size;
	int logfd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
	memset(&addr, 0, sizeof(struct sockaddr_un));
	/* Clear structure */
	    addr.sun_family = AF_UNIX;
	    strncpy(addr.sun_path, syslog_path,
	    sizeof(addr.sun_path) - 1);

	   addr_size = sizeof(struct sockaddr_un);
	    int ret = connect(logfd, (struct sockaddr *) &addr,
	 addr_size);
	    if (ret == -1)
	handle_error("connect");
	return logfd;
}
