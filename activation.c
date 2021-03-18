#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ini.h>
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
	char* name;
	char* systemd_service;
	char* user;

	int id;
};

tll(struct service) service_list = tll_init();

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

static int handler(void* user, const char* section, const char* name, const char* value) {
	struct service* service = user;
	if (MATCH("D-BUS Service", "Name")) {
		service->name = strdup(value);
	} else if (MATCH("D-BUS Service", "SystemdService")) {
		service->systemd_service = strdup(value);
	} else if (MATCH("D-BUS Service", "User")) {
		service->user = strdup(value);
	} else return 0;
	return 1;
}

void add_dir(const char* path) {
	struct dirent **namelist;
	int n;

	n = scandir(path, &namelist, NULL, alphasort);
	if (n == -1) {
		perror("scandir");
		exit(EXIT_FAILURE);
	}

	chdir(path);
	while (n--) {
		if (namelist[n]->d_name[0] != '.') {
			struct service service = {0};
			ini_parse(namelist[n]->d_name, handler, &service);
			if (service.name && strcmp(service.name, "org.bluez")) {
				fprintf(stderr, "%s\n", service.name);
				tll_push_back(service_list, service);
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
		sd_bus_call_method(bus_controller, NULL, "/org/bus1/DBus/Broker", "org.bus1.DBus.Broker", "AddName", NULL, NULL, "osu", path, it->item.name, 0);
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
		//printf("%s\n", service->systemd_service);
		char cmd[256];
		sprintf(cmd, "s6-rc -l %s change %s", s6rc_livedir, service->name);
		fprintf(stderr, "%s\n", cmd);
		r = system(cmd);
	}
	return r;
}
