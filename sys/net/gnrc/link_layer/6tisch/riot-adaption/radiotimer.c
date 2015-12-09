#include "stdint.h"

#include "periph/timer.h"

#include "radiotimer.h"
#include "board_info.h"

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
   radiotimer_capture_cbt    start_frame_cb;
   radiotimer_capture_cbt    end_frame_cb;
   uint8_t                   overflowORcompare;
   uint32_t                  currentSlotPeriod;
} radiotimer_vars_t;

volatile radiotimer_vars_t radiotimer_vars;

/* assumption: timer runs with 1MHz */
#define TIMER_TICKS(x)       ((x)*1000)
#define TIMER_TICKS_TO_US(x) ((x)/1000)

//=========================== prototypes ======================================
extern int timer_set_relative(tim_t, int channel, unsigned int rel_value);
//=========================== public ==========================================

//===== admin

void radiotimer_init(void) {
   // clear local variables
   memset((void*)&radiotimer_vars,0,sizeof(radiotimer_vars_t));
}

void radiotimer_setOverflowCb(radiotimer_compare_cbt cb) {
   radiotimer_vars.overflow_cb    = cb;
}

void radiotimer_setCompareCb(radiotimer_compare_cbt cb) {
   radiotimer_vars.compare_cb     = cb;
}

void radiotimer_setStartFrameCb(radiotimer_capture_cbt cb) {
   radiotimer_vars.start_frame_cb = cb;
}

void radiotimer_setEndFrameCb(radiotimer_capture_cbt cb) {
   radiotimer_vars.end_frame_cb = cb;
}

void radiotimer_start(PORT_RADIOTIMER_WIDTH period) {
    DEBUG("%s\n", __PRETTY_FUNCTION__);
    timer_init(OWSN_TIMER, 1, &radiotimer_isr);
    timer_set(OWSN_TIMER, 0, ((unsigned int)TIMER_TICKS(period)));
    radiotimer_vars.currentSlotPeriod = period;
}

//===== direct access

PORT_RADIOTIMER_WIDTH radiotimer_getValue(void) {
    return (PORT_RADIOTIMER_WIDTH)(TIMER_TICKS_TO_US(timer_read(OWSN_TIMER)));
}

void radiotimer_setPeriod(PORT_RADIOTIMER_WIDTH period) {
    DEBUG("%s\n", __PRETTY_FUNCTION__);
    timer_set(OWSN_TIMER, 0, ((unsigned int)TIMER_TICKS(period)));
    radiotimer_vars.currentSlotPeriod = period;
}

PORT_RADIOTIMER_WIDTH radiotimer_getPeriod(void) {
    return radiotimer_vars.currentSlotPeriod;
}

//===== compare

void radiotimer_schedule(PORT_RADIOTIMER_WIDTH offset) {
    DEBUG("%s\n", __PRETTY_FUNCTION__);
    timer_set(OWSN_TIMER, 1, TIMER_TICKS(offset));
}

void radiotimer_cancel(void) {
    DEBUG("%s\n", __PRETTY_FUNCTION__);
    timer_set(OWSN_TIMER, 1, 0);
}

//===== capture

inline PORT_RADIOTIMER_WIDTH radiotimer_getCapturedTime(void) {
    return (PORT_RADIOTIMER_WIDTH)(timer_read(OWSN_TIMER));
}

//=========================== private =========================================

//=========================== interrupt handlers ==============================
void radiotimer_isr(int channel) {
    if (channel == 0) {
      DEBUG("%s cmp\n", __PRETTY_FUNCTION__);
      if (radiotimer_vars.compare_cb!=NULL) {
          radiotimer_vars.compare_cb();
      }
    }
    else {
      DEBUG("%s of\n", __PRETTY_FUNCTION__);
      if (radiotimer_vars.overflow_cb!=NULL) {
          timer_set(OWSN_TIMER, 1, 0);
          radiotimer_vars.overflow_cb();
      }
    }
}