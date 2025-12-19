#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "writer.h"

void write_to_lv(const char *mount_point) {
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/fichier%d.txt", mount_point, rand() % 100000);

    int size_mb = rand() % 91 + 10; // 10–100 MB
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "fallocate -l %dM %s", size_mb, filename);
    (void)system(cmd);

    printf("Wrote %dMB to %s\n", size_mb, filename);
}
