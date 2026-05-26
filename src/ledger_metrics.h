#ifndef _LEDGER_METRICS_H_
#define _LEDGER_METRICS_H_

enum ledger_metrics_subsystem
{
    LEDGER_SUBSYSTEM_COMBAT = 0,
    LEDGER_SUBSYSTEM_ECONOMY,
    LEDGER_SUBSYSTEM_QUEST,
    LEDGER_SUBSYSTEM_LIFECYCLE,
    LEDGER_SUBSYSTEM_MAX
};

enum ledger_metrics_event_type
{
    LEDGER_EVENT_CHARACTER_DEATH = 0,
    LEDGER_EVENT_EXP_DELTA,
    LEDGER_EVENT_QUEST_COMPLETE,
    LEDGER_EVENT_SHOP_BUY,
    LEDGER_EVENT_SHOP_SELL,
    LEDGER_EVENT_RENT_SAVE,
    LEDGER_EVENT_MAX
};

void ledger_metrics_init(void);
void ledger_metrics_shutdown(void);
void ledger_metrics_on_pulse(unsigned long heart_pulse);
void ledger_metrics_record_event(enum ledger_metrics_subsystem subsystem, enum ledger_metrics_event_type event_type,
                                 unsigned int estimated_size_bytes);
void ledger_metrics_record_event_in_room(enum ledger_metrics_subsystem subsystem,
                                         enum ledger_metrics_event_type event_type, unsigned int estimated_size_bytes,
                                         room_rnum room);
void ledger_metrics_record_rng(enum ledger_metrics_subsystem subsystem, unsigned int calls);
/* Wrapper de rand_number() para contabilizar uso de RNG por subsistema sem alterar o resultado do sorteio. */
int ledger_rand_number(enum ledger_metrics_subsystem subsystem, int from, int to);

#endif
