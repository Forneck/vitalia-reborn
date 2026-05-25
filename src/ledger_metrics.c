#include "conf.h"
#include "sysdep.h"

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "ledger_metrics.h"

#define LEDGER_METRICS_FLUSH_SECONDS (15 * 60)
#define LEDGER_METRICS_FLUSH_PULSES (LEDGER_METRICS_FLUSH_SECONDS * PASSES_PER_SEC)
#define LEDGER_METRICS_SIZE_BUCKETS 6

struct ledger_metrics_subsystem_data {
    unsigned long long events;
    unsigned long long estimated_bytes;
    unsigned long long rng_calls;
};

struct ledger_metrics_data {
    unsigned long long event_counts[LEDGER_EVENT_MAX];
    unsigned long long event_size_buckets[LEDGER_EVENT_MAX][LEDGER_METRICS_SIZE_BUCKETS];
    struct ledger_metrics_subsystem_data subsystem[LEDGER_SUBSYSTEM_MAX];
    unsigned long interval_pulses;
    unsigned long interval_mud_ticks;
};

static struct ledger_metrics_data interval_data;
static struct ledger_metrics_data total_data;
static FILE *metrics_file = NULL;
static bool metrics_enabled = FALSE;
static bool open_failure_logged = FALSE;

static const char *subsystem_names[LEDGER_SUBSYSTEM_MAX] = {"combat", "economy", "quest", "lifecycle"};
static const char *event_names[LEDGER_EVENT_MAX] = {"character_death", "exp_delta", "quest_complete",
                                                     "shop_buy", "shop_sell", "rent_save"};
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

static bool has_interval_data(void)
{
    int i;
    if (interval_data.interval_pulses > 0 || interval_data.interval_mud_ticks > 0) {
        for (i = 0; i < LEDGER_SUBSYSTEM_MAX; i++) {
            if (interval_data.subsystem[i].events || interval_data.subsystem[i].estimated_bytes ||
                interval_data.subsystem[i].rng_calls)
                return TRUE;
        }
    }
    return FALSE;
}

static void reset_interval_data(void)
{
    memset(&interval_data, 0, sizeof(interval_data));
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
    int bucket;

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

    total_data.subsystem[subsystem].events++;
    total_data.subsystem[subsystem].estimated_bytes += estimated_size_bytes;
    total_data.event_counts[event_type]++;
    total_data.event_size_buckets[event_type][bucket]++;
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
