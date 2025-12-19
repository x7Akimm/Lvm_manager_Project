#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define NUM_LVS 3

const char *pv_devices[] = {"/dev/loop15", "/dev/loop16"};
const char *lv_names[NUM_LVS] = {"vol1", "vol2", "vol3"};
const char *mount_points[NUM_LVS] = {"/mnt/mountlv1", "/mnt/mountlv2", "/mnt/mountlv3"};
const char *vg_name = "vGroup";

void run_cmd(const char *cmd) {
    int ret = system(cmd);
    if (ret != 0) {
        fprintf(stderr, "[ERROR] Command failed: %s\n", cmd);
        exit(EXIT_FAILURE);
    }
}

int lv_exists(const char *lv_path) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sudo lvdisplay %s >/dev/null 2>&1", lv_path);
    return system(cmd) == 0;
}

int vg_exists(const char *vg) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sudo vgdisplay %s >/dev/null 2>&1", vg);
    return system(cmd) == 0;
}

void create_pv_vg_lv() {
    // pv creation
    for (int i = 0; i < sizeof(pv_devices)/sizeof(pv_devices[0]); i++) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "sudo pvcreate -ff %s >/dev/null 2>&1", pv_devices[i]);
        run_cmd(cmd);
        printf("Physical volume \"%s\" created.\n", pv_devices[i]);
    }

    // create and extending vg
    if (!vg_exists(vg_name)) {
        char cmd[512] = {0};
        snprintf(cmd, sizeof(cmd), "sudo vgcreate %s", vg_name);
        for (int i = 0; i < sizeof(pv_devices)/sizeof(pv_devices[0]); i++)
            snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), " %s", pv_devices[i]);
        run_cmd(cmd);
        printf("Volume group \"%s\" created.\n", vg_name);
    } else {
        // extend vg or pv
        for (int i = 0; i < sizeof(pv_devices)/sizeof(pv_devices[0]); i++) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "sudo vgextend %s %s >/dev/null 2>&1", vg_name, pv_devices[i]);
            system(cmd);
        }
    }

    // create LVS
    for (int i = 0; i < NUM_LVS; i++) {
        char lv_path[256];
        snprintf(lv_path, sizeof(lv_path), "/dev/%s/%s", vg_name, lv_names[i]);
        if (!lv_exists(lv_path)) {
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "sudo lvcreate -n %s -L 500M %s >/dev/null 2>&1", lv_names[i], vg_name);
            run_cmd(cmd);
            printf("Logical volume \"%s\" created.\n", lv_names[i]);
        }
    }
}

void format_and_mount() {
    for (int i = 0; i < NUM_LVS; i++) {
        char lv_path[256];
        snprintf(lv_path, sizeof(lv_path), "/dev/%s/%s", vg_name, lv_names[i]);

        // creat mount point
        mkdir(mount_points[i], 0755);

        char cmd[256];
        snprintf(cmd, sizeof(cmd), "sudo blkid %s >/dev/null 2>&1 || sudo mkfs.ext4 %s", lv_path, lv_path);
        run_cmd(cmd);
        // mount if its not already mounted
        snprintf(cmd, sizeof(cmd), "mountpoint -q %s || sudo mount %s %s", mount_points[i], lv_path, mount_points[i]);
        run_cmd(cmd);
    }
    printf("PV, VG, LVs created and mounted.\n");
}

void wipe_all() {
    char cmd[512];

    // unmount lv (for the clean build commande in Makefile)
    for (int i = 0; i < NUM_LVS; i++) {
        snprintf(cmd, sizeof(cmd), "sudo umount -f %s >/dev/null 2>&1", mount_points[i]);
        system(cmd); // ignore errors
    }

    // remove lvs
    for (int i = 0; i < NUM_LVS; i++) {
        snprintf(cmd, sizeof(cmd), "sudo lvremove -y /dev/%s/%s >/dev/null 2>&1", vg_name, lv_names[i]);
        system(cmd);
    }

    // remove vgs
    snprintf(cmd, sizeof(cmd), "sudo vgremove -y %s >/dev/null 2>&1", vg_name);
    system(cmd);

    // remove pvs
    for (int i = 0; i < sizeof(pv_devices)/sizeof(pv_devices[0]); i++) {
        snprintf(cmd, sizeof(cmd), "sudo pvremove -y %s >/dev/null 2>&1", pv_devices[i]);
        system(cmd);
    }

    printf("All PV, VG, LVs wiped.\n");
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "wipe") == 0) {
        wipe_all();
        return 0;
    }

    create_pv_vg_lv();
    format_and_mount();
    return 0;
}
