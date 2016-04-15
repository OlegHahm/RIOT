#include <sys/uio.h>
#include "board_info.h"
#include "radio.h"
#include "board.h"
#include "xtimer.h"
#include "radiotimer.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/netapi.h"
#include "net/gnrc/netreg.h"
#include "led.h"

#define ENABLE_DEBUG (0)
#include "debug.h"


//=========================== defines =========================================


//=========================== variables =======================================

typedef struct {
   radiotimer_capture_cbt    startFrame_cb;
   radiotimer_capture_cbt    endFrame_cb;
   radio_state_t             state;
   netdev2_t                 *dev;
} radio_vars_t;

radio_vars_t radio_vars;

#ifndef MODULE_AT86RF2XX
//=========================== prototypes ======================================
// static void event_cb(gnrc_netdev_event_t event, void *data);

//=========================== public ==========================================

//===== admin

/*
 * Radio already initialised by RIOT's auto_init process, using at86rf2xx_init.
 */

void radio_init(gnrc_netdev2_t *dev_par) {
   DEBUG("%s\n", __PRETTY_FUNCTION__);
   // clear variables
   memset(&radio_vars,0,sizeof(radio_vars_t));
   radio_vars.state = RADIOSTATE_STOPPED;
   radio_vars.dev = dev_par->dev;

   radio_vars.dev->driver->init(radio_vars.dev);
   netopt_enable_t enable;
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_PROMISCUOUSMODE, &(enable), sizeof(netopt_enable_t));
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_RX_START_IRQ, &(enable), sizeof(netopt_enable_t));
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_RX_END_IRQ, &(enable), sizeof(netopt_enable_t));
#ifndef CPU_NATIVE
   enable = NETOPT_ENABLE;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_TX_END_IRQ, &(enable), sizeof(netopt_enable_t));
#endif
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
   struct iovec vector;
   vector.iov_base = packet;
   vector.iov_len = len;
   // load packet in TXFIFO
   radio_vars.dev->driver->send(radio_vars.dev, &vector, 1);

   // change state
   radio_vars.state = RADIOSTATE_PACKET_LOADED;
}

void radio_txEnable(void) {
   // change state
   radio_vars.state = RADIOSTATE_ENABLING_TX;

   // change state
   radio_vars.state = RADIOSTATE_TX_ENABLED;
}

#ifdef CPU_NATIVE
xtimer_t _tx_timer;

static void _tx_complete_isr(void* arg)
{
    (void) arg;
    if (radio_vars.dev->event_callback) {
        radio_vars.dev->event_callback(radio_vars.dev, NETDEV2_EVENT_TX_COMPLETE, NULL);
    }
}
#endif

void radio_txNow(void) {
   PORT_TIMER_WIDTH val;
   // change state
   radio_vars.state = RADIOSTATE_TRANSMITTING;

   netopt_state_t state = NETOPT_STATE_TX;
   radio_vars.dev->driver->set(radio_vars.dev, NETOPT_STATE, &(state), sizeof(netopt_state_t));
#ifdef CPU_NATIVE
    _tx_timer.callback = &_tx_complete_isr;
    xtimer_set(&_tx_timer, 3600);
#endif

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
void event_cb(netdev2_t *dev, netdev2_event_t type, void *data)
{
    (void) dev;
   // capture the time
   uint32_t capturedTime = radiotimer_getCapturedTime();

   // start of frame event
   if (type == NETDEV2_EVENT_RX_STARTED) {
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
   if (type == NETDEV2_EVENT_RX_COMPLETE
       || type == NETDEV2_EVENT_TX_COMPLETE) {
       DEBUG("End of Frame.\n");
      // change state
      radio_vars.state = RADIOSTATE_TXRX_DONE;
      if (radio_vars.endFrame_cb!=NULL) {
         // call the callback
         radio_vars.endFrame_cb(capturedTime);
      } else {
         while(1);
      }
      if (type == NETDEV2_EVENT_RX_COMPLETE) {
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

#else

#include "at86rf2xx.h"
#include "at86rf2xx_internal.h"
#include "at86rf2xx_registers.h"

void radio_spiReadRxFifo(uint8_t* pBufRead,
                         uint8_t* pLenRead,
                         uint8_t  maxBufLen,
                         uint8_t* pLqi);
//===== admin

void radio_init(gnrc_netdev2_t *dev_par) {

    // clear variables
    memset(&radio_vars,0,sizeof(radio_vars_t));

    // change state
    radio_vars.state          = RADIOSTATE_STOPPED;
    radio_vars.dev = dev_par->dev;
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;

    /* initialise GPIOs */
    gpio_init(dev->params.cs_pin, GPIO_OUT);
    gpio_set(dev->params.cs_pin);
    gpio_init(dev->params.sleep_pin, GPIO_OUT);
    gpio_clear(dev->params.sleep_pin);
    gpio_init(dev->params.reset_pin, GPIO_OUT);
    gpio_set(dev->params.reset_pin);
    gpio_init_int(dev->params.int_pin, GPIO_IN, GPIO_RISING, radio_isr, dev);

    // configure the radio
    at86rf2xx_reg_write(dev, AT86RF2XX_REG__TRX_STATE, AT86RF2XX_TRX_STATE__FORCE_TRX_OFF);    // turn radio off

    at86rf2xx_reg_write(dev, AT86RF2XX_REG__IRQ_MASK,
                        (AT86RF2XX_IRQ_STATUS_MASK__RX_START| AT86RF2XX_IRQ_STATUS_MASK__TRX_END));  // tell radio to fire interrupt on TRX_END and RX_START
    at86rf2xx_reg_read(dev, AT86RF2XX_REG__IRQ_STATUS);                       // deassert the interrupt pin in case is high
    at86rf2xx_reg_write(dev, AT86RF2XX_REG__ANT_DIV, 0x04);     // use chip antenna
    at86rf2xx_reg_write(dev, AT86RF2XX_REG__TRX_CTRL_1, 0x20);                // have the radio calculate CRC
    //busy wait until radio status is TRX_OFF

    while((at86rf2xx_reg_read(dev, AT86RF2XX_REG__TRX_STATUS) & 0x1F) != AT86RF2XX_STATE_TRX_OFF);

    // change state
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
    PORT_PIN_RADIO_RESET_LOW();
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
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;
    // change state
    radio_vars.state = RADIOSTATE_SETTING_FREQUENCY;

    // configure the radio to the right frequecy
    at86rf2xx_reg_write(dev, AT86RF2XX_REG__PHY_CC_CCA,0x20+frequency);

    // change state
    radio_vars.state = RADIOSTATE_FREQUENCY_SET;
}

void radio_rfOn(void) {
    PORT_PIN_RADIO_RESET_LOW();
}

void radio_rfOff(void) {
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;
    // change state
    radio_vars.state = RADIOSTATE_TURNING_OFF;
    at86rf2xx_reg_read(dev, AT86RF2XX_REG__TRX_STATUS);
    // turn radio off
    at86rf2xx_reg_write(dev, AT86RF2XX_REG__TRX_STATE, AT86RF2XX_TRX_STATE__FORCE_TRX_OFF);
    //at86rf2xx_reg_write(dev, AT86RF2XX_REG__TRX_STATE, CMD_TRX_OFF);
    while((at86rf2xx_reg_read(dev, AT86RF2XX_REG__TRX_STATUS) & 0x1F) != AT86RF2XX_STATE_TRX_OFF); // busy wait until done

    LED1_OFF;

    // change state
    radio_vars.state = RADIOSTATE_RFOFF;
}

//===== TX

void radio_loadPacket(uint8_t* packet, uint8_t len) {
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;
    // change state
    radio_vars.state = RADIOSTATE_LOADING_PACKET;

    // load packet in TXFIFO
    at86rf2xx_tx_load(dev, packet, len, 0);

    // change state
    radio_vars.state = RADIOSTATE_PACKET_LOADED;
}

void radio_txEnable(void) {
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;
    // change state
    radio_vars.state = RADIOSTATE_ENABLING_TX;

    // wiggle debug pin
    LED1_ON;

    // turn on radio's PLL
    at86rf2xx_reg_write(dev, AT86RF2XX_REG__TRX_STATE, AT86RF2XX_TRX_STATE__PLL_ON);
    while((at86rf2xx_reg_read(dev, AT86RF2XX_REG__TRX_STATUS) & 0x1F) != AT86RF2XX_TRX_STATUS__PLL_ON); // busy wait until done

    // change state
    radio_vars.state = RADIOSTATE_TX_ENABLED;
}

void radio_txNow(void) {
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;
    PORT_TIMER_WIDTH val;
    // change state
    radio_vars.state = RADIOSTATE_TRANSMITTING;

    // send packet by pulsing the SLP_TR_CNTL pin
    at86rf2xx_reg_write(dev, AT86RF2XX_REG__TRX_STATE,
                        AT86RF2XX_TRX_STATE__TX_START);

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
}

//===== RX

void radio_rxEnable(void) {
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;
    // change state
    radio_vars.state = RADIOSTATE_ENABLING_RX;

    // put radio in reception mode
    at86rf2xx_reg_write(dev, AT86RF2XX_REG__TRX_STATE, AT86RF2XX_TRX_STATE__RX_ON);

    // wiggle debug pin
    LED1_ON;

    // busy wait until radio really listening
    while((at86rf2xx_reg_read(dev, AT86RF2XX_REG__TRX_STATUS) & 0x1F) != AT86RF2XX_TRX_STATUS__RX_ON);

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
    uint8_t temp_reg_value;
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;

    //===== crc
    temp_reg_value  = at86rf2xx_reg_read(dev, AT86RF2XX_REG__PHY_RSSI);
    *pCrc           = (temp_reg_value & 0x80)>>7;  // msb is whether packet passed CRC

    //===== rssi
    // as per section 8.4.3 of the AT86RF231, the RSSI is calculate as:
    // -91 + ED [dBm]
    temp_reg_value  = at86rf2xx_reg_read(dev, AT86RF2XX_REG__PHY_ED_LEVEL);
    *pRssi          = -91 + temp_reg_value;

    //===== packet
    radio_spiReadRxFifo(pBufRead,
                        pLenRead,
                        maxBufLen,
                        pLqi);
}



void radio_spiReadRxFifo(uint8_t* pBufRead,
                         uint8_t* pLenRead,
                         uint8_t  maxBufLen,
                         uint8_t* pLqi) {
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;
    // when reading the packet over SPI from the RX buffer, you get the following:
    // - *[1B]     dummy byte because of SPI
    // - *[1B]     length byte
    // -  [0-125B] packet (excluding CRC)
    // - *[2B]     CRC
    // - *[1B]     LQI
    uint8_t spi_tx_buffer[125];
    uint8_t spi_rx_buffer[3];

    spi_tx_buffer[0] = 0x20;

    spi_acquire(dev->params.spi);
    gpio_clear(dev->params.cs_pin);
    // 2 first bytes
    spi_transfer_bytes(dev->params.spi,
                       spi_tx_buffer,
                       spi_rx_buffer,
                       2
                      );

    *pLenRead  = spi_rx_buffer[1];

    if (*pLenRead>2 && *pLenRead<=127) {
        // valid length

        //read packet
        spi_transfer_bytes(dev->params.spi,
                           spi_tx_buffer,
                           pBufRead,
                           *pLenRead
                          );

        // CRC (2B) and LQI (1B)
        spi_transfer_bytes(dev->params.spi,
                           spi_tx_buffer,
                           spi_rx_buffer,
                           2+1
                          );

        *pLqi   = spi_rx_buffer[2];

    } else {
        // invalid length

        // read a just byte to close spi
        spi_transfer_bytes(dev->params.spi,
                           spi_tx_buffer,
                           spi_rx_buffer,
                           1
                          );
    }
    gpio_set(dev->params.cs_pin);
    spi_release(dev->params.spi);
}

//=========================== callbacks =======================================

//=========================== interrupt handlers ==============================

void radio_isr(void *unused) {
    at86rf2xx_t *dev = (at86rf2xx_t*) radio_vars.dev;
    PORT_TIMER_WIDTH capturedTime;
    uint8_t  irq_status;

    // capture the time
    capturedTime = radiotimer_getCapturedTime();

    // reading IRQ_STATUS causes radio's IRQ pin to go low
    irq_status = at86rf2xx_reg_read(dev, AT86RF2XX_REG__IRQ_STATUS);

    // start of frame event
    if (irq_status & AT86RF2XX_IRQ_STATUS_MASK__RX_START) {
        // change state
        radio_vars.state = RADIOSTATE_RECEIVING;
        if (radio_vars.startFrame_cb!=NULL) {
            // call the callback
            radio_vars.startFrame_cb(capturedTime);
            // kick the OS
            return;
        }
    }
    // end of frame event
    if (irq_status & AT86RF2XX_IRQ_STATUS_MASK__TRX_END) {
        // change state
        radio_vars.state = RADIOSTATE_TXRX_DONE;
        if (radio_vars.endFrame_cb!=NULL) {
            // call the callback
            radio_vars.endFrame_cb(capturedTime);
            // kick the OS
            return;
        }
    }

    return;
}
#endif
