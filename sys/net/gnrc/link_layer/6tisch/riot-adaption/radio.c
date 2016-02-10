#include "board_info.h"
#include "radio.h"
#include "board.h"
#include "radiotimer.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/netreg.h"

#define ENABLE_DEBUG (0)
#include "debug.h"


//=========================== defines =========================================

//=========================== variables =======================================

typedef struct {
   radiotimer_capture_cbt    startFrame_cb;
   radiotimer_capture_cbt    endFrame_cb;
   radio_state_t             state;
   gnrc_netdev_t             *dev;
} radio_vars_t;

radio_vars_t radio_vars;

//=========================== prototypes ======================================
static void event_cb(gnrc_netdev_event_t event, void *data);

//=========================== public ==========================================

//===== admin

/*
 * Radio already initialised by RIOT's auto_init process, using at86rf2xx_init.
 */

void radio_init(gnrc_netdev_t *dev_par) {
   DEBUG("%s\n", __PRETTY_FUNCTION__);
   // clear variables
   memset(&radio_vars,0,sizeof(radio_vars_t));
   radio_vars.state = RADIOSTATE_STOPPED;
   radio_vars.dev = dev_par;

   netopt_enable_t enable;
   radio_vars.dev->driver->add_event_callback(radio_vars.dev, event_cb);
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_PROMISCUOUSMODE, &(enable), sizeof(netopt_enable_t));
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_RX_START_IRQ, &(enable), sizeof(netopt_enable_t));
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_RX_END_IRQ, &(enable), sizeof(netopt_enable_t));
   enable = NETOPT_DISABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_AUTOACK, &(enable), sizeof(netopt_enable_t));
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_PRELOADING, &(enable), sizeof(netopt_enable_t));
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_RAWMODE, &(enable), sizeof(netopt_enable_t));
   uint8_t retrans = 0;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_RETRANS, &(retrans), sizeof(uint8_t));

   radio_vars.state          = RADIOSTATE_RFOFF;
}

void radio_setOverflowCb(radiotimer_compare_cbt cb) {
   radiotimer_setOverflowCb(cb);
}

void radio_setCompareCb(radiotimer_compare_cbt cb) {
   radiotimer_setCompareCb(cb);
}

void radio_setStartFrameCb(radiotimer_capture_cbt cb) {
   radio_vars.startFrame_cb  = cb;
}

void radio_setEndFrameCb(radiotimer_capture_cbt cb) {
   radio_vars.endFrame_cb    = cb;
}

//===== reset

void radio_reset(void) {
   netopt_state_t state = NETOPT_STATE_RESET;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_STATE, &(state), sizeof(netopt_state_t));
}

//===== timer

void radio_startTimer(PORT_TIMER_WIDTH period) {
   radiotimer_start(period);
}

PORT_TIMER_WIDTH radio_getTimerValue(void) {
   return radiotimer_getValue();
}

void radio_setTimerPeriod(PORT_TIMER_WIDTH period) {
   radiotimer_setPeriod(period);
}

PORT_TIMER_WIDTH radio_getTimerPeriod(void) {
   return radiotimer_getPeriod();
}

//===== RF admin

void radio_setFrequency(uint8_t frequency) {
   // change state
   radio_vars.state = RADIOSTATE_SETTING_FREQUENCY;

   // configure the radio to the right frequency
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_CHANNEL, &(frequency), sizeof(uint8_t));

   // change state
   radio_vars.state = RADIOSTATE_FREQUENCY_SET;
}

void radio_rfOn(void) {
   netopt_state_t state = NETOPT_STATE_IDLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_STATE, &(state), sizeof(netopt_state_t));
}

void radio_rfOff(void) {
   netopt_state_t state = NETOPT_STATE_OFF;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_STATE, &(state), sizeof(netopt_state_t));
}

//===== TX

void radio_loadPacket(uint8_t* packet, uint8_t len) {
   DEBUG("rf load\n");
   // change state
   radio_vars.state = RADIOSTATE_LOADING_PACKET;

   /* wrap data into pktsnip */
   gnrc_pktsnip_t *pkt;
   pkt = gnrc_pktbuf_add(NULL, packet, len, GNRC_NETTYPE_UNDEF);
   // load packet in TXFIFO
   radio_vars.dev->driver->send_data(radio_vars.dev, pkt);

   // change state
   radio_vars.state = RADIOSTATE_PACKET_LOADED;
}

void radio_txEnable(void) {
   // change state
   radio_vars.state = RADIOSTATE_ENABLING_TX;

   // change state
   radio_vars.state = RADIOSTATE_TX_ENABLED;
}

void radio_txNow(void) {
   PORT_TIMER_WIDTH val;
   // change state
   radio_vars.state = RADIOSTATE_TRANSMITTING;

   netopt_state_t state = NETOPT_STATE_TX;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_STATE, &(state), sizeof(netopt_state_t));

   // The AT86RF231 does not generate an interrupt when the radio transmits the
   // SFD, which messes up the MAC state machine. The danger is that, if we leave
   // this funtion like this, any radio watchdog timer will expire.
   // Instead, we cheat an mimick a start of frame event by calling
   // ieee154e_startOfFrame from here. This also means that software can never catch
   // a radio glitch by which #radio_txEnable would not be followed by a packet being
   // transmitted (I've never seen that).
   if (radio_vars.startFrame_cb!=NULL) {
      // call the callback
      val=radiotimer_getCapturedTime();
      radio_vars.startFrame_cb(val);
   }
   DEBUG("SENT");
}

//===== RX

void radio_rxEnable(void) {
   // change state
   radio_vars.state = RADIOSTATE_ENABLING_RX;

   netopt_state_t state = NETOPT_STATE_IDLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_STATE, &(state), sizeof(netopt_state_t));

   // change state
   radio_vars.state = RADIOSTATE_LISTENING;
}

void radio_rxNow(void) {
   // nothing to do
}

void radio_getReceivedFrame(uint8_t* pBufRead,
                            uint8_t* pLenRead,
                            uint8_t  maxBufLen,
                             int8_t* pRssi,
                            uint8_t* pLqi,
                               bool* pCrc) {
    (void) pBufRead;
    (void) pLenRead;
    (void) maxBufLen;
    (void) pRssi;
    (void) pLqi;
    (void) pCrc;
   /* The driver notifies the MAC with the registered
      `event_cb` that a new packet arrived.
      I think this function should not be used to access
      the new packet.
      RSSI and LQI are already parsed into the netif portion
      of the pktsnip. */
}

//=========================== callbacks =======================================

//=========================== interrupt handlers ==============================

static void event_cb(gnrc_netdev_event_t type, void *data)
{
   // capture the time
   uint32_t capturedTime = radiotimer_getCapturedTime();

   // start of frame event
   if (type == NETDEV_EVENT_RX_STARTED) {
       DEBUG("Start of frame.\n");
      // change state
      radio_vars.state = RADIOSTATE_RECEIVING;
      if (radio_vars.startFrame_cb!=NULL) {
         // call the callback
         radio_vars.startFrame_cb(capturedTime);
      } else {
         while(1);
      }
   }
   // end of frame event
   if (type == NETDEV_EVENT_RX_COMPLETE
       || type == NETDEV_EVENT_TX_COMPLETE) {
       DEBUG("End of Frame.\n");
      // change state
      radio_vars.state = RADIOSTATE_TXRX_DONE;
      if (radio_vars.endFrame_cb!=NULL) {
         // call the callback
         radio_vars.endFrame_cb(capturedTime);
      } else {
         while(1);
      }
      if (type == NETDEV_EVENT_RX_COMPLETE) {
         gnrc_pktsnip_t *pkt;

        /* get pointer to the received packet */
        pkt = (gnrc_pktsnip_t *)data;
        /* send the packet to everyone interested in it's type */
        if (!gnrc_netapi_dispatch_receive(pkt->type, GNRC_NETREG_DEMUX_CTX_ALL, pkt)) {
            DEBUG("6TiSCH: unable to forward packet of type %i\n", pkt->type);
            gnrc_pktbuf_release(pkt);
        }
      }
   }
}