#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PARTITIONS 3
#define LIMIT 90
#define EXTEND_SIZE_MB 100  // size in MB

const char *lv_paths[NUM_PARTITIONS] = {
    "/dev/vGroup/vol1",
    "/dev/vGroup/vol2",
    "/dev/vGroup/vol3"
};

const char *mount_points[NUM_PARTITIONS] = {
    "/mnt/mountlv1",
    "/mnt/mountlv2",
    "/mnt/mountlv3"
};

// main function for the process2 SUPERVISION
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

// extend lv by size_mb
static void extend_lv(const char *lv_path, int size_mb) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "sudo lvextend -L+%dM %s && sudo resize2fs %s",
             size_mb, lv_path, lv_path);
    printf("[ACTION] Extending LV %s by %dMB...\n", lv_path, size_mb);
    system(cmd);
}

// shrink a logical volume by size_mb 
static void shrink_lv(const char *lv_path, int size_mb) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "sudo umount %s && sudo e2fsck -f %s && sudo resize2fs %s %dM && sudo lvreduce -L-%dM %s -y && sudo mount %s %s",
             lv_path, lv_path, lv_path, size_mb, size_mb, lv_path, lv_path, lv_path);
    printf("[ACTION] Shrinking LV %s by %dMB...\n", lv_path, size_mb);
    system(cmd);
}

void balance_lvs() {
    for (int i = 0; i < NUM_PARTITIONS; i++) {
        int usage = get_usage_percent(mount_points[i]);
        if (usage <= LIMIT) continue;

        printf("[BALANCER] LV %s (%s) usage %d%% exceeds limit %d%%\n",
               lv_paths[i], mount_points[i], usage, LIMIT);

        // get space from the vg if is not completely used
        char cmd[256];
        snprintf(cmd, sizeof(cmd),
                 "vgs --noheadings -o vg_free vGroup | tr -d ' '");
        FILE *fp = popen(cmd, "r");
        if (!fp) continue;
        char vg_free[64];
        if (fgets(vg_free, sizeof(vg_free), fp)) {
            double free_mb = atof(vg_free) * 1024; // convert to mb
            if (free_mb >= EXTEND_SIZE_MB) {
                extend_lv(lv_paths[i], EXTEND_SIZE_MB);
                pclose(fp);
                continue;
            }
        }
        pclose(fp);

        // try to shrink other lvs IN THE SAME group to get free space for the lv who needs it
        for (int j = 0; j < NUM_PARTITIONS; j++) {
            if (j == i) continue;
            int other_usage = get_usage_percent(mount_points[j]);
            if (other_usage < 50) { // only shrink lv who didnt use more than 50% of storage
                printf("[BALANCER] Shrinking LV %s to free space\n", lv_paths[j]);
                shrink_lv(lv_paths[j], 100); // shrink 100 MB
            }
        }

        // if this warning appear means that either the lvm isn't working or all the lv are almost saturated
        usage = get_usage_percent(mount_points[i]);
        if (usage > LIMIT) {
            printf("[BALANCER] Warning: LV %s (%s) still above limit! Manual intervention needed.\n",
                   lv_paths[i], mount_points[i]);
        }
    }
}
