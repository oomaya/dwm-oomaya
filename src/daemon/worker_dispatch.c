/* worker_dispatch.c — Subsystem dispatch implementation for worker threads.
 *
 * Handles sysfs power queries and monitor cycling.
 * Executes on worker threads to protect reactor fast-path latency (<10µs).
 */
#define _POSIX_C_SOURCE 200809L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>

#include "../../include/oomaya_ipc.h"
#include "../../include/oomaya_ring.h"
#include "../../include/oomaya_state.h"
#include "../../include/oomaya_dispatch.h"

static int32_t query_power(void) {
    glob_t gl;
    int32_t ret = -1;
    const char *power_dir = getenv("DWM_STATUS_POWER_SUPPLY_DIR");
    char pattern[256];

    if (power_dir && power_dir[0] != '\0') {
        snprintf(pattern, sizeof(pattern), "%s/BAT*", power_dir);
    } else {
        snprintf(pattern, sizeof(pattern), "/sys/class/power_supply/BAT*");
    }

    if (glob(pattern, 0, NULL, &gl) == 0 && gl.gl_pathc > 0) {
        char path[256];
        snprintf(path, sizeof(path), "%s/capacity", gl.gl_pathv[0]);
        FILE *f = fopen(path, "r");
        if (f) {
            int cap = 0;
            if (fscanf(f, "%d", &cap) == 1) ret = (int32_t)cap;
            fclose(f);
        }
    }
    globfree(&gl);
    if (ret >= 0) return ret;

    /* Check AC */
    if (power_dir && power_dir[0] != '\0') {
        snprintf(pattern, sizeof(pattern), "%s/AC*", power_dir);
    } else {
        snprintf(pattern, sizeof(pattern), "/sys/class/power_supply/AC*");
    }

    if (glob(pattern, 0, NULL, &gl) == 0 && gl.gl_pathc > 0) {
        char path[256];
        snprintf(path, sizeof(path), "%s/online", gl.gl_pathv[0]);
        FILE *f = fopen(path, "r");
        if (f) {
            int online = 0;
            if (fscanf(f, "%d", &online) == 1 && online == 1) ret = 100;
            fclose(f);
        }
    }
    globfree(&gl);
    return (ret >= 0) ? ret : 100;
}

void
oomaya_default_dispatch(oomaya_task_t *task, oomaya_wm_state_t *state)
{
    if (task == NULL) return;
    (void)state;

    task->status = 0;

    switch (task->opcode) {
    case IPC_CMD_SYS_GET_POWER: {
        int32_t pwr = query_power();
        task->resp_len = sizeof(pwr);
        memcpy(task->resp_data, &pwr, sizeof(pwr));
        break;
    }
    case IPC_CMD_WM_CYCLE_MONITOR: {
        int rc = system("xdotool key Super+period >/dev/null 2>&1");
        if (rc != 0) {
            task->status   = OOMAYA_IPC_ERR_INTERNAL;
            task->resp_len = 0;
            break;
        }
        task->resp_len = sizeof(int32_t);
        memcpy(task->resp_data, &task->status, sizeof(int32_t));
        break;
    }
    /* A nonzero status is an oomaya_ipc_err_t; the reactor carries it in the
     * error frame's payload_len with no body. */
    default: {
        task->status   = OOMAYA_IPC_ERR_OPCODE;
        task->resp_len = 0;
        break;
    }
    }
}
