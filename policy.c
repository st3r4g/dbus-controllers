#include "policy.h"

#include <stdbool.h>

#define POLICY_T_BATCH                                                          \
                "bt"                                                            \
                "a(btbs)"                                                       \
                "a(btssssuutt)"                                                 \
                "a(btssssuutt)"

#define POLICY_T                                                                \
                "a(u(" POLICY_T_BATCH "))"                                      \
                "a(buu(" POLICY_T_BATCH "))"                                    \
                "a(ss)"                                                         \
                "b"

#define POLICY_PRIORITY_DEFAULT (UINT64_C(1))

void policy_export_connect(sd_bus_message *m) {
        sd_bus_message_append(m, "bt", true, POLICY_PRIORITY_DEFAULT);
}

void policy_export_own(sd_bus_message *m) {
	sd_bus_message_open_container(m, 'a', "(btbs)");
	sd_bus_message_append(m, "(btbs)", true, POLICY_PRIORITY_DEFAULT, true, "");
        sd_bus_message_close_container(m);
}

void policy_export_xmit(sd_bus_message *m) {
	sd_bus_message_open_container(m, 'a', "(btssssuutt)");
	sd_bus_message_append(m, "(btssssuutt)", true, POLICY_PRIORITY_DEFAULT, "", "", "", "", 0, 0, 0, 0);
        sd_bus_message_close_container(m);
}

int policy(sd_bus_message *m) {
	int r;
        r = sd_bus_message_open_container(m, 'v', "(" POLICY_T ")");
        r = sd_bus_message_open_container(m, 'r', POLICY_T);
        r = sd_bus_message_open_container(m, 'a', "(u(" POLICY_T_BATCH "))");
        r = sd_bus_message_open_container(m, 'r', "u(" POLICY_T_BATCH ")");
        r = sd_bus_message_append(m, "u", (uint32_t)-1);
        r = sd_bus_message_open_container(m, 'r', POLICY_T_BATCH);
	policy_export_connect(m);
        policy_export_own(m);
        policy_export_xmit(m);
        policy_export_xmit(m);

        r = sd_bus_message_close_container(m);
        r = sd_bus_message_close_container(m);
        r = sd_bus_message_close_container(m);

        r = sd_bus_message_open_container(m, 'a', "(buu(" POLICY_T_BATCH "))");
        r = sd_bus_message_close_container(m);

	r = sd_bus_message_open_container(m, 'a', "(ss)");
        r = sd_bus_message_close_container(m);

        r = sd_bus_message_append(m, "b", false);

        r = sd_bus_message_close_container(m);
        r = sd_bus_message_close_container(m);
        return r;
}
