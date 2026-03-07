#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define BMS_NUM_CELLS 140
#define BMS_NUM_DCC   10

struct bms_data_t {
    float total_v;
    float max_v;
    float min_v;
    float avg_v;
    float max_min_v;

    float cell_v[BMS_NUM_CELLS];
    int   num_cells;

    float max_temp;
    float min_temp;
    float avg_temp;

    int undervoltage;
    int overvoltage;
    int pec;
    int overtemperature;
    int openwire;
    int openwire_temp;

    int dcc[BMS_NUM_DCC];

    uint32_t time_ms;
    float current;
    float soc;

    bool valid;
};

extern bms_data_t bms;

void bms_feed(const uint8_t *data, size_t len);

// Returns averaged BMS data accumulated since last call, then resets the accumulator.
// If no samples were accumulated, falls back to the latest raw snapshot.
void bms_accum_snapshot(bms_data_t *out);

// Number of samples accumulated since last snapshot.
int bms_accum_count();
