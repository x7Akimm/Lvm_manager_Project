#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "writer.h"
#include "balancer.h"
#include "config.h"

#define LIMIT 90

extern const char *lv_paths[NUM_PARTITIONS];s
extern const char *mount_points[NUM_PARTITIONS];


static int get_usage_percent(const char *path) {
    char cmd[256], result[64];
    snprintf(cmd, sizeof(cmd),
             "df -h %s | tail -1 | awk '{print $5}' | tr -d '%%\\n'", path);
    FILE *fp = popen(cmd, "r");
    if (!fp) return 0;
    if (!fgets(result, sizeof(result), fp)) {
        pclose(fp);
        return 0;
    }
    pclose(fp);
    return atoi(result);
}

void *writer_thread(void *arg) {
    (void)arg;
    while (1) {
        for (int i = 0; i < NUM_PARTITIONS; i++) {
            write_to_lv(mount_points[i]);
        }
        sleep(60);  
    }
    return NULL;
}


void *balancer_thread(void *arg) {
    (void)arg;
    balance_lvs();  
    return NULL;
}


void *supervisor_thread(void *arg) {
    (void)arg;
    while (1) {
        for (int i = 0; i < NUM_PARTITIONS; i++) {
            int usage = get_usage_percent(mount_points[i]);
            if (usage > LIMIT) {
                printf("[SUPERVISOR] LV %s usage %d%% > %d%%, spawning balancer thread...\n",
                       lv_paths[i], usage, LIMIT);

                pthread_t tid;
                if (pthread_create(&tid, NULL, balancer_thread, NULL) == 0) {
                    pthread_detach(tid);  // run independently
                } else {
                    fprintf(stderr, "[ERROR] Failed to create balancer thread\n");
                }
            }
        }
        sleep(60); 
    }
    return NULL;
}


int main() {
    srand(time(NULL));

    pthread_t writer_tid, supervisor_tid;

  
    if (pthread_create(&writer_tid, NULL, writer_thread, NULL) != 0) {
        fprintf(stderr, "[ERROR] Failed to create writer thread\n");
        return 1;
    }

    
    if (pthread_create(&supervisor_tid, NULL, supervisor_thread, NULL) != 0) {
        fprintf(stderr, "[ERROR] Failed to create supervisor thread\n");
        return 1;
    }

 
    pthread_join(writer_tid, NULL);
    pthread_join(supervisor_tid, NULL);

    return 0;
}
