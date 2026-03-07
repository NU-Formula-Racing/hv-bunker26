#include "bms_data.h"
#include <string.h>
#include <stdio.h>

bms_data_t bms = {};

// ---------------------------------------------------------------------------
// Accumulator — collects samples between POSTs and produces an average
// ---------------------------------------------------------------------------

static struct {
    float total_v, max_v, min_v, avg_v, max_min_v;
    float cell_v[BMS_NUM_CELLS];
    float max_temp, min_temp, avg_temp;
    float current, soc;

    int undervoltage, overvoltage, pec, overtemperature, openwire, openwire_temp;
    int dcc[BMS_NUM_DCC];
    int num_cells;
    uint32_t time_ms;

    int count;
} accum = {};

static void accum_add()
{
    accum.total_v   += bms.total_v;
    accum.max_v     += bms.max_v;
    accum.min_v     += bms.min_v;
    accum.avg_v     += bms.avg_v;
    accum.max_min_v += bms.max_min_v;

    for (int i = 0; i < bms.num_cells; i++)
        accum.cell_v[i] += bms.cell_v[i];

    accum.max_temp += bms.max_temp;
    accum.min_temp += bms.min_temp;
    accum.avg_temp += bms.avg_temp;
    accum.current  += bms.current;
    accum.soc      += bms.soc;

    if (bms.undervoltage)    accum.undervoltage    = 1;
    if (bms.overvoltage)     accum.overvoltage     = 1;
    if (bms.pec)             accum.pec             = 1;
    if (bms.overtemperature) accum.overtemperature  = 1;
    if (bms.openwire)        accum.openwire        = 1;
    if (bms.openwire_temp)   accum.openwire_temp   = 1;

    if (bms.num_cells > accum.num_cells) accum.num_cells = bms.num_cells;
    accum.time_ms = bms.time_ms;
    memcpy(accum.dcc, bms.dcc, sizeof(accum.dcc));

    accum.count++;
}

void bms_accum_snapshot(bms_data_t *out)
{
    if (accum.count == 0) {
        *out = bms;
        return;
    }

    float n = (float)accum.count;

    out->total_v   = accum.total_v   / n;
    out->max_v     = accum.max_v     / n;
    out->min_v     = accum.min_v     / n;
    out->avg_v     = accum.avg_v     / n;
    out->max_min_v = accum.max_min_v / n;

    out->num_cells = accum.num_cells;
    for (int i = 0; i < out->num_cells; i++)
        out->cell_v[i] = accum.cell_v[i] / n;

    out->max_temp = accum.max_temp / n;
    out->min_temp = accum.min_temp / n;
    out->avg_temp = accum.avg_temp / n;
    out->current  = accum.current  / n;
    out->soc      = accum.soc      / n;

    out->undervoltage    = accum.undervoltage;
    out->overvoltage     = accum.overvoltage;
    out->pec             = accum.pec;
    out->overtemperature = accum.overtemperature;
    out->openwire        = accum.openwire;
    out->openwire_temp   = accum.openwire_temp;

    memcpy(out->dcc, accum.dcc, sizeof(out->dcc));
    out->time_ms = accum.time_ms;
    out->valid   = true;

    memset(&accum, 0, sizeof(accum));
}

int bms_accum_count()
{
    return accum.count;
}

// ---------------------------------------------------------------------------
// Line parser — matches each incoming text line to a BMS field
// ---------------------------------------------------------------------------

static void parse_line(const char *line)
{
    const char *p;

    if (strncmp(line, "total_v:", 8) == 0) {
        sscanf(line, "total_v: %f", &bms.total_v);
        if ((p = strstr(line, "max_v:")))   sscanf(p, "max_v: %f",   &bms.max_v);
        if ((p = strstr(line, "min_v:")))   sscanf(p, "min_v: %f",   &bms.min_v);
        if ((p = strstr(line, "avg_v:")))   sscanf(p, "avg_v: %f",   &bms.avg_v);
        if ((p = strstr(line, "max-min:"))) sscanf(p, "max-min: %f", &bms.max_min_v);
        bms.valid = true;
        accum_add();
    }
    else if (line[0] == 'C' && line[1] >= '1' && line[1] <= '9') {
        p = line;
        bms.num_cells = 0;
        while (*p) {
            if (*p == 'C') {
                int cell_num;
                float voltage;
                if (sscanf(p, "C%d=%f", &cell_num, &voltage) == 2 &&
                    cell_num >= 1 && cell_num <= BMS_NUM_CELLS) {
                    bms.cell_v[cell_num - 1] = voltage;
                    if (cell_num > bms.num_cells) bms.num_cells = cell_num;
                }
            }
            while (*p && *p != '\t') p++;
            while (*p == '\t') p++;
        }
    }
    else if (strncmp(line, "max_temp:", 9) == 0) {
        sscanf(line, "max_temp: %f", &bms.max_temp);
        if ((p = strstr(line, "min_temp:"))) sscanf(p, "min_temp: %f", &bms.min_temp);
        if ((p = strstr(line, "avg_temp:"))) sscanf(p, "avg_temp: %f", &bms.avg_temp);
    }
    else if (strncmp(line, "undervoltage:", 13) == 0) {
        sscanf(line, "undervoltage: %d", &bms.undervoltage);
        if ((p = strstr(line, "overvoltage:")))     sscanf(p, "overvoltage: %d",     &bms.overvoltage);
        if ((p = strstr(line, "pec:")))             sscanf(p, "pec: %d",             &bms.pec);
        if ((p = strstr(line, "overtemperature:"))) sscanf(p, "overtemperature: %d",  &bms.overtemperature);
        if ((p = strstr(line, "openwire:")))        sscanf(p, "openwire: %d",        &bms.openwire);
        if ((p = strstr(line, "openwire_temp:")))   sscanf(p, "openwire_temp: %d",   &bms.openwire_temp);
    }
    else if (strncmp(line, "dcc1:", 5) == 0) {
        p = line;
        for (int i = 0; i < BMS_NUM_DCC; i++) {
            char key[8];
            snprintf(key, sizeof(key), "dcc%d:", i + 1);
            const char *found = strstr(p, key);
            if (found) {
                sscanf(found + strlen(key), " %d", &bms.dcc[i]);
                p = found + 1;
            }
        }
    }
    else if (strncmp(line, "Time:", 5) == 0) {
        sscanf(line, "Time: %u", &bms.time_ms);
    }
    else if (strncmp(line, "Current:", 8) == 0) {
        sscanf(line, "Current: %f", &bms.current);
    }
    else if (strncmp(line, "SOC:", 4) == 0) {
        sscanf(line, "SOC: %f", &bms.soc);
        bms.valid = true;
    }
}

// ---------------------------------------------------------------------------
// Feed — accumulates raw USB bytes into lines, then parses each line
// ---------------------------------------------------------------------------

static char line_buf[3072];
static size_t line_pos = 0;

void bms_feed(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        char c = (char)data[i];
        if (c == '\n' || c == '\r') {
            if (line_pos > 0) {
                line_buf[line_pos] = '\0';
                parse_line(line_buf);
                line_pos = 0;
            }
        } else if (line_pos < sizeof(line_buf) - 1) {
            line_buf[line_pos++] = c;
        }
    }
}
