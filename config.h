#ifndef CONFIG_H
#define CONFIG_H

#define NUM_LVS 3
#define NUM_PARTITIONS NUM_LVS   

extern const char *lv_names[NUM_LVS];
extern const char *mount_points[NUM_LVS];
extern const char *pv_device;

#endif
