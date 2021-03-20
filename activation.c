#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <skalibs/djbunix.h>

#include <tllist.h>

/*int service_add(Service *service) {
        Launcher *launcher = service->launcher;
        _c_cleanup_(c_freep) char *object_path = NULL;
        int r;

        if (service->running)
                return 0;

        r = asprintf(&object_path, "/org/bus1/DBus/Name/%s", service->id);
        if (r < 0)
                return error_origin(-ENOMEM);

        r = sd_bus_call_method(launcher->bus_controller,
                               NULL,
                               "/org/bus1/DBus/Broker",
                               "org.bus1.DBus.Broker",
                               "AddName",
                               NULL,
                               NULL,
                               "osu",
                               object_path,
                               service->name,
                               service->uid);
        if (r < 0)
                return error_origin(r);

        service->running = true;
        return 0;
}*/

struct service {
	stralloc name;
	stralloc s6rc;

	int id;
};

tll(struct service) service_list = tll_init();

#define NAMEFILE "data/dbus-activatable-name"
#define SERVICEDIRS "servicedirs"
#define SIZE(array) (sizeof(array)/sizeof(*array))

void add_s6rc_servicedirs(const char* s6rc_livedir) {
	struct dirent **namelist;
	int n;

	size_t s6rc_livedir_len = strlen(s6rc_livedir);
	char path[s6rc_livedir_len+1+SIZE(SERVICEDIRS)];
	memcpy(path, s6rc_livedir, s6rc_livedir_len);
	path[s6rc_livedir_len] = '/';
	memcpy(path+s6rc_livedir_len+1, SERVICEDIRS, SIZE(SERVICEDIRS));
	n = scandir(path, &namelist, NULL, alphasort);
	if (n == -1) {
		perror("scandir");
		exit(EXIT_FAILURE);
	}

	chdir(path); // TODO: remove this, the directory could be deleted by s6-rc-update
	while (n--) {
		if (namelist[n]->d_name[0] != '.') {
			struct service service = {0};
			size_t d_len = strlen(namelist[n]->d_name);
			char tmp[d_len+1+SIZE(NAMEFILE)];
			memcpy(tmp, namelist[n]->d_name, d_len);
			tmp[d_len] = '/';
			memcpy(tmp+d_len+1, NAMEFILE, SIZE(NAMEFILE));

			int r = openslurpclose(&service.name, tmp);
			if (r == 1 && service.name.len > 0) {
				service.name.s[service.name.len-1] = '\0';
				stralloc_copyb(&service.s6rc, namelist[n]->d_name, d_len+1);
				tll_push_back(service_list, service);
				fprintf(stderr, "Activatable name %s provided by %s\n", service.name.s, service.s6rc.s);
			}
		}

		free(namelist[n]);
	}
	free(namelist);
}

#include "sd-bus.h"

void add_service(sd_bus* bus_controller) {
	int i=0;
        tll_foreach(service_list, it) {
	        char path[128];
	        sprintf(path, "/org/bus1/DBus/Name/%d", i);
		sd_bus_call_method(bus_controller, NULL, "/org/bus1/DBus/Broker", "org.bus1.DBus.Broker", "AddName", NULL, NULL, "osu", path, it->item.name.s, 0);
		it->item.id = i;
		i++;
        }
}

static struct service *find_service_by_id(int id) {
        tll_foreach(service_list, it)
	        if (it->item.id == id)
		        return &it->item;
        return NULL;
}

const char* s6rc_livedir; // TODO: pass around properly

int start_service(int id) {
	int r = -1;
	struct service* service = find_service_by_id(id);
	if (service) {
		char cmd[256];
		sprintf(cmd, "s6-rc -l %s change %s", s6rc_livedir, service->s6rc.s);
		fprintf(stderr, "%s\n", cmd);
		r = system(cmd);
	}
	return r;
}
