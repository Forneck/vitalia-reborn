#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "ledger_metrics.h"

#define LEDGER_METRICS_FLUSH_SECONDS (15 * 60)
#define LEDGER_METRICS_FLUSH_PULSES (LEDGER_METRICS_FLUSH_SECONDS * PASSES_PER_SEC)
#define LEDGER_METRICS_SIZE_BUCKETS 6
#define LEDGER_METRICS_HOURS 24
#define LEDGER_METRICS_TOP_ZONES 8

struct ledger_metrics_subsystem_data {
    unsigned long long events;
    unsigned long long estimated_bytes;
    unsigned long long rng_calls;
};

struct ledger_metrics_data {
    unsigned long long event_counts[LEDGER_EVENT_MAX];
    unsigned long long event_size_buckets[LEDGER_EVENT_MAX][LEDGER_METRICS_SIZE_BUCKETS];
    unsigned long long event_hour_counts[LEDGER_METRICS_HOURS];
    struct ledger_metrics_subsystem_data subsystem[LEDGER_SUBSYSTEM_MAX];
    unsigned long interval_pulses;
    unsigned long interval_mud_ticks;
};

static struct ledger_metrics_data interval_data;
static struct ledger_metrics_data total_data;
static FILE *metrics_file = NULL;
static bool metrics_enabled = FALSE;
static bool open_failure_logged = FALSE;
static unsigned long long *interval_zone_events = NULL;
static unsigned long long *interval_zone_bytes = NULL;
static unsigned long long *total_zone_events = NULL;
static unsigned long long *total_zone_bytes = NULL;
static size_t zone_metrics_count = 0;

static const char *subsystem_names[LEDGER_SUBSYSTEM_MAX] = {"combat", "economy", "quest", "lifecycle"};
static const char *event_names[LEDGER_EVENT_MAX] = {"character_death", "exp_delta", "quest_complete",
                                                    "shop_buy",        "shop_sell", "rent_save"};
static const unsigned int size_bucket_limits[LEDGER_METRICS_SIZE_BUCKETS - 1] = {64, 128, 256, 512, 1024};

static int get_size_bucket(unsigned int estimated_size_bytes)
{
    int i;
    for (i = 0; i < (LEDGER_METRICS_SIZE_BUCKETS - 1); i++) {
        if (estimated_size_bytes <= size_bucket_limits[i])
            return i;
    }
    return (LEDGER_METRICS_SIZE_BUCKETS - 1);
}

static zone_rnum get_zone_from_room(room_rnum room)
{
    zone_rnum zone;

    if (room == NOWHERE || room < 0 || room > top_of_world)
        return NOWHERE;

    zone = world[room].zone;
    if (zone < 0 || zone > top_of_zone_table)
        return NOWHERE;

    return zone;
}

static void reset_interval_data(void)
{
    memset(&interval_data, 0, sizeof(interval_data));
    if (interval_zone_events && zone_metrics_count > 0)
        memset(interval_zone_events, 0, zone_metrics_count * sizeof(*interval_zone_events));
    if (interval_zone_bytes && zone_metrics_count > 0)
        memset(interval_zone_bytes, 0, zone_metrics_count * sizeof(*interval_zone_bytes));
}

static bool has_interval_data(void)
{
    int i;
    if (interval_data.interval_pulses > 0 || interval_data.interval_mud_ticks > 0) {
        for (i = 0; i < LEDGER_SUBSYSTEM_MAX; i++) {
            if (interval_data.subsystem[i].events || interval_data.subsystem[i].estimated_bytes ||
                interval_data.subsystem[i].rng_calls)
                return TRUE;
        }
        for (i = 0; i < LEDGER_METRICS_HOURS; i++)
            if (interval_data.event_hour_counts[i] > 0)
                return TRUE;
    }
    return FALSE;
}

static void flush_interval_data(unsigned long pulse, bool force)
{
    int i, j;
    time_t now;

    if (!metrics_enabled || !metrics_file)
        return;

    if (!force && interval_data.interval_pulses < LEDGER_METRICS_FLUSH_PULSES)
        return;

    if (!has_interval_data()) {
        reset_interval_data();
        return;
    }

    now = time(0);
    fprintf(metrics_file, "ts=%ld pulse=%lu interval_pulses=%lu interval_mud_ticks=%lu", (long)now, pulse,
            interval_data.interval_pulses, interval_data.interval_mud_ticks);

    for (i = 0; i < LEDGER_SUBSYSTEM_MAX; i++) {
        fprintf(metrics_file, " %s_events=%llu %s_bytes=%llu %s_rng=%llu", subsystem_names[i],
                interval_data.subsystem[i].events, subsystem_names[i], interval_data.subsystem[i].estimated_bytes,
                subsystem_names[i], interval_data.subsystem[i].rng_calls);
    }

    for (i = 0; i < LEDGER_EVENT_MAX; i++) {
        fprintf(metrics_file, " %s_count=%llu", event_names[i], interval_data.event_counts[i]);
        for (j = 0; j < LEDGER_METRICS_SIZE_BUCKETS; j++) {
            fprintf(metrics_file, " %s_bucket%d=%llu", event_names[i], j, interval_data.event_size_buckets[i][j]);
        }
    }

    for (i = 0; i < LEDGER_METRICS_HOURS; i++)
        if (interval_data.event_hour_counts[i] > 0)
            fprintf(metrics_file, " hour_%d=%llu", i, interval_data.event_hour_counts[i]);

    if (interval_zone_events && interval_zone_bytes && zone_metrics_count > 0) {
        zone_rnum top_zone_index[LEDGER_METRICS_TOP_ZONES];
        unsigned long long top_zone_events[LEDGER_METRICS_TOP_ZONES];
        unsigned long long top_zone_bytes[LEDGER_METRICS_TOP_ZONES];

        memset(top_zone_index, 0, sizeof(top_zone_index));
        memset(top_zone_events, 0, sizeof(top_zone_events));
        memset(top_zone_bytes, 0, sizeof(top_zone_bytes));

        for (i = 0; i < (int)zone_metrics_count; i++) {
            unsigned long long current_events = interval_zone_events[i];
            unsigned long long current_bytes;
            int pos;

            if (current_events == 0)
                continue;

            current_bytes = interval_zone_bytes[i];
            for (pos = 0; pos < LEDGER_METRICS_TOP_ZONES; pos++) {
                int shift;
                if (current_events <= top_zone_events[pos])
                    continue;
                for (shift = LEDGER_METRICS_TOP_ZONES - 1; shift > pos; shift--) {
                    top_zone_events[shift] = top_zone_events[shift - 1];
                    top_zone_bytes[shift] = top_zone_bytes[shift - 1];
                    top_zone_index[shift] = top_zone_index[shift - 1];
                }
                top_zone_events[pos] = current_events;
                top_zone_bytes[pos] = current_bytes;
                top_zone_index[pos] = i;
                break;
            }
        }

        for (i = 0; i < LEDGER_METRICS_TOP_ZONES; i++)
            if (top_zone_events[i] > 0)
                fprintf(metrics_file, " zone_hot%d=%d:%llu:%llu", i, zone_table[top_zone_index[i]].number,
                        top_zone_events[i], top_zone_bytes[i]);
    }

    fprintf(metrics_file, "\n");
    fflush(metrics_file);
    reset_interval_data();
}

void ledger_metrics_init(void)
{
    if (metrics_enabled)
        return;

    metrics_file = fopen(PREFIX_LOGFILE "ledger_metrics", "a");
    if (!metrics_file) {
        if (!open_failure_logged) {
            log1("WARN: ledger metrics disabled (failed to open %s).", PREFIX_LOGFILE "ledger_metrics");
            open_failure_logged = TRUE;
        }
        metrics_enabled = FALSE;
        return;
    }

    setvbuf(metrics_file, NULL, _IOLBF, 0);
    memset(&interval_data, 0, sizeof(interval_data));
    memset(&total_data, 0, sizeof(total_data));

    zone_metrics_count = (top_of_zone_table >= 0) ? ((size_t)top_of_zone_table + 1) : 0;
    if (zone_metrics_count > 0) {
        interval_zone_events = calloc(zone_metrics_count, sizeof(*interval_zone_events));
        interval_zone_bytes = calloc(zone_metrics_count, sizeof(*interval_zone_bytes));
        total_zone_events = calloc(zone_metrics_count, sizeof(*total_zone_events));
        total_zone_bytes = calloc(zone_metrics_count, sizeof(*total_zone_bytes));

        if (!interval_zone_events || !interval_zone_bytes || !total_zone_events || !total_zone_bytes) {
            free(interval_zone_events);
            free(interval_zone_bytes);
            free(total_zone_events);
            free(total_zone_bytes);
            interval_zone_events = NULL;
            interval_zone_bytes = NULL;
            total_zone_events = NULL;
            total_zone_bytes = NULL;
            zone_metrics_count = 0;
            log1("WARN: ledger metrics zone hotspots disabled (allocation failed).");
        }
    }

    metrics_enabled = TRUE;
}

void ledger_metrics_shutdown(void)
{
    if (!metrics_enabled)
        return;

    flush_interval_data(total_data.interval_pulses, TRUE);

    if (metrics_file) {
        fclose(metrics_file);
        metrics_file = NULL;
    }
    free(interval_zone_events);
    free(interval_zone_bytes);
    free(total_zone_events);
    free(total_zone_bytes);
    interval_zone_events = NULL;
    interval_zone_bytes = NULL;
    total_zone_events = NULL;
    total_zone_bytes = NULL;
    zone_metrics_count = 0;
    metrics_enabled = FALSE;
}

void ledger_metrics_on_pulse(unsigned long heart_pulse)
{
    if (!metrics_enabled)
        return;

    interval_data.interval_pulses++;
    total_data.interval_pulses++;

    if (!(heart_pulse % (SECS_PER_MUD_HOUR * PASSES_PER_SEC))) {
        interval_data.interval_mud_ticks++;
        total_data.interval_mud_ticks++;
    }

    if (interval_data.interval_pulses >= LEDGER_METRICS_FLUSH_PULSES)
        flush_interval_data(heart_pulse, FALSE);
}

void ledger_metrics_record_event(enum ledger_metrics_subsystem subsystem, enum ledger_metrics_event_type event_type,
                                 unsigned int estimated_size_bytes)
{
    ledger_metrics_record_event_in_room(subsystem, event_type, estimated_size_bytes, NOWHERE);
}

void ledger_metrics_record_event_in_room(enum ledger_metrics_subsystem subsystem,
                                         enum ledger_metrics_event_type event_type, unsigned int estimated_size_bytes,
                                         room_rnum room)
{
    int bucket;
    zone_rnum zone;
    int hour;

    if (!metrics_enabled)
        return;
    if (subsystem < 0 || subsystem >= LEDGER_SUBSYSTEM_MAX)
        return;
    if (event_type < 0 || event_type >= LEDGER_EVENT_MAX)
        return;

    bucket = get_size_bucket(estimated_size_bytes);

    interval_data.subsystem[subsystem].events++;
    interval_data.subsystem[subsystem].estimated_bytes += estimated_size_bytes;
    interval_data.event_counts[event_type]++;
    interval_data.event_size_buckets[event_type][bucket]++;
    hour = time_info.hours;
    if (hour < 0)
        hour = 0;
    hour %= LEDGER_METRICS_HOURS;
    interval_data.event_hour_counts[hour]++;
    zone = get_zone_from_room(room);
    if (zone >= 0 && (size_t)zone < zone_metrics_count && interval_zone_events && interval_zone_bytes) {
        interval_zone_events[zone]++;
        interval_zone_bytes[zone] += estimated_size_bytes;
    }

    total_data.subsystem[subsystem].events++;
    total_data.subsystem[subsystem].estimated_bytes += estimated_size_bytes;
    total_data.event_counts[event_type]++;
    total_data.event_size_buckets[event_type][bucket]++;
    total_data.event_hour_counts[hour]++;
    if (zone >= 0 && (size_t)zone < zone_metrics_count && total_zone_events && total_zone_bytes) {
        total_zone_events[zone]++;
        total_zone_bytes[zone] += estimated_size_bytes;
    }
}

void ledger_metrics_record_rng(enum ledger_metrics_subsystem subsystem, unsigned int calls)
{
    if (!metrics_enabled)
        return;
    if (subsystem < 0 || subsystem >= LEDGER_SUBSYSTEM_MAX)
        return;
    if (calls == 0)
        return;

    interval_data.subsystem[subsystem].rng_calls += calls;
    total_data.subsystem[subsystem].rng_calls += calls;
}

int ledger_rand_number(enum ledger_metrics_subsystem subsystem, int from, int to)
{
    ledger_metrics_record_rng(subsystem, 1);
    return rand_number(from, to);
}
