#include "util.h"

#include <stdio.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

int dbus_create_socket(const char* dbus_socket_path) {
	const int backlog = 50;
	int sfd;
	struct sockaddr_un my_addr;

	sfd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sfd == -1)
		handle_error("socket");

	memset(&my_addr, 0, sizeof(my_addr));
                               /* Clear structure */
           my_addr.sun_family = AF_UNIX;
           strncpy(my_addr.sun_path, dbus_socket_path,
                   sizeof(my_addr.sun_path) - 1);

	mode_t old_mode = umask(0000);
           if (bind(sfd, (struct sockaddr *) &my_addr,
                   sizeof(my_addr)) == -1)
               handle_error("bind");
	umask(old_mode);

           if (listen(sfd, backlog) == -1)
               handle_error("listen");
           return sfd;
}
