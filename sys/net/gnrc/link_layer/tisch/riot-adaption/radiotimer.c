#include "stdint.h"

#include "xtimer.h"

#include "radiotimer.h"
#include "board_info.h"
#include "IEEE802154E.h"

// #include "riot.h"

#define ENABLE_DEBUG (0)
#include "debug.h"


//=========================== variables =======================================

enum  radiotimer_irqstatus_enum{
    RADIOTIMER_NONE     = 0x00, //alarm interrupt default status
    RADIOTIMER_OVERFLOW = 0x01, //alarm interrupt caused by overflow
    RADIOTIMER_COMPARE  = 0x02, //alarm interrupt caused by compare
};

typedef struct {
   radiotimer_compare_cbt    overflow_cb;
   radiotimer_compare_cbt    compare_cb;
   uint32_t                  currentSlotPeriod;
   xtimer_t                  p_timer;
   xtimer_t                  s_timer;
} radiotimer_vars_t;

radiotimer_vars_t radiotimer_vars;
extern ieee154e_vars_t    ieee154e_vars;

void radiotimer_init(void) {
   // clear local variables
   memset((void*)&radiotimer_vars,0,sizeof(radiotimer_vars_t));
   radiotimer_vars.p_timer.callback = &radiotimer_isr_p;
   radiotimer_vars.s_timer.callback = &radiotimer_isr_s;
}

void radiotimer_setOverflowCb(radiotimer_compare_cbt cb) {
   radiotimer_vars.overflow_cb    = cb;
}

void radiotimer_setCompareCb(radiotimer_compare_cbt cb) {
   radiotimer_vars.compare_cb     = cb;
}

void radiotimer_start(PORT_RADIOTIMER_WIDTH period) {
    DEBUG("%s\n", "radiotimer");
    // timer_init(OWSN_TIMER, 1, &radiotimer_isr);
    // timer_set(OWSN_TIMER, 0, ((unsigned int)TIMER_TICKS(period)));
    
    radiotimer_vars.currentSlotPeriod = period;
    xtimer_set(&(radiotimer_vars.p_timer), (uint32_t) period);
}

//===== direct access

PORT_RADIOTIMER_WIDTH radiotimer_getValue(void) {
    return (PORT_RADIOTIMER_WIDTH) (xtimer_now() - ieee154e_vars.slotStartTS);
}

void radiotimer_setPeriod(PORT_RADIOTIMER_WIDTH period) {
    DEBUG("radiotimer_setPeriod: %u, %u\n", period, ieee154e_vars.slotStartTS);
    // timer_set(OWSN_TIMER, 0, ((unsigned int)TIMER_TICKS(period)));
    
    radiotimer_vars.currentSlotPeriod = period;
    xtimer_set(&(radiotimer_vars.p_timer), (uint32_t) period);
}

PORT_RADIOTIMER_WIDTH radiotimer_getPeriod(void) {
    return radiotimer_vars.currentSlotPeriod;
}

//===== compare

void radiotimer_schedule(PORT_RADIOTIMER_WIDTH offset) {
    DEBUG("%s schedule\n", "radiotimer");
    // timer_set(OWSN_TIMER, 1, TIMER_TICKS(offset));
    xtimer_set(&(radiotimer_vars.s_timer), (uint32_t) (offset));
}

void radiotimer_cancel(void) {
    DEBUG("%s cancel\n", "radiotimer");
    // timer_set(OWSN_TIMER, 1, 0);
    xtimer_remove(&(radiotimer_vars.s_timer));
    xtimer_set(&(radiotimer_vars.s_timer), radiotimer_vars.currentSlotPeriod);
}

//===== capture

inline PORT_RADIOTIMER_WIDTH radiotimer_getCapturedTime(void) {
    return (PORT_RADIOTIMER_WIDTH)(xtimer_now() - ieee154e_vars.slotStartTS);
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================
void radiotimer_isr_s(void* arg) {
    (void) arg;
    DEBUG("%s cmp\n", "radiotimer");
    if (radiotimer_vars.compare_cb!=NULL) {
        radiotimer_vars.compare_cb();
    }
}

void radiotimer_isr_p(void* arg) {
    (void) arg;
    DEBUG("%s cmp\n", "radiotimer");
    if (radiotimer_vars.overflow_cb != NULL) {
        radiotimer_vars.overflow_cb();
    }
    ieee154e_vars.slotStartTS = xtimer_now();
}
