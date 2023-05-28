#include "AsyncAdc.h"

#include <Arduino.h>

#ifdef ESP32

#include "esp32-hal-adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h"
#include "esp_attr.h"
#include "esp_intr.h"
#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#elif defined(NRF52840_XXAA) || defined(NRF52) || defined(NRF52_SERIES)

#include "nrf.h"
#include "wiring_private.h"
#include "wiring_analog.h"

#define IRAM_ATTR

static uint32_t saadcReference  = SAADC_CH_CONFIG_REFSEL_Internal;
static uint32_t saadcGain       = SAADC_CH_CONFIG_GAIN_Gain1_6;
static uint32_t saadcSampleTime = SAADC_CH_CONFIG_TACQ_5us;

static bool saadcBurst = SAADC_CH_CONFIG_BURST_Disabled;

static int readResolution = 10;
static uint32_t lastResolution = 10;

static uint8_t adc_state_machine = 0;
static volatile uint16_t value_cache;

static inline uint32_t mapResolution(uint32_t value, uint32_t from, uint32_t to)
{
  if ( from == to )
  {
    return value ;
  }

  if ( from > to )
  {
    return value >> (from-to) ;
  }
  else
  {
    return value << (to-from) ;
  }
}

/*
 * Internal Reference is +/-0.6V, with an adjustable gain of 1/6, 1/5, 1/4,
 * 1/3, 1/2 or 1, meaning 3.6, 3.0, 2.4, 1.8, 1.2 or 0.6V for the ADC levels.
 *
 * External Reference is VDD/4, with an adjustable gain of 1, 2 or 4, meaning
 * VDD/4, VDD/2 or VDD for the ADC levels.
 *
 * Default settings are internal reference with 1/6 gain (GND..3.6V ADC range)
 *
 * Warning : On Arduino Zero board the input/output voltage for SAMD21G18 is 3.3 volts maximum
 */
void adcSetReference(eAnalogReference ulMode)
{
  switch ( ulMode ) {
    case AR_VDD4:
      saadcReference = SAADC_CH_CONFIG_REFSEL_VDD1_4;
      saadcGain      = SAADC_CH_CONFIG_GAIN_Gain1_4;
      break;
    case AR_INTERNAL_3_0:
      saadcReference = SAADC_CH_CONFIG_REFSEL_Internal;
      saadcGain      = SAADC_CH_CONFIG_GAIN_Gain1_5;
      break;
    case AR_INTERNAL_2_4:
      saadcReference = SAADC_CH_CONFIG_REFSEL_Internal;
      saadcGain      = SAADC_CH_CONFIG_GAIN_Gain1_4;
      break;
    case AR_INTERNAL_1_8:
      saadcReference = SAADC_CH_CONFIG_REFSEL_Internal;
      saadcGain      = SAADC_CH_CONFIG_GAIN_Gain1_3;
      break;
    case AR_INTERNAL_1_2:
      saadcReference = SAADC_CH_CONFIG_REFSEL_Internal;
      saadcGain      = SAADC_CH_CONFIG_GAIN_Gain1_2;
      break;
    case AR_DEFAULT:
    case AR_INTERNAL:
    default:
      saadcReference = SAADC_CH_CONFIG_REFSEL_Internal;
      saadcGain      = SAADC_CH_CONFIG_GAIN_Gain1_6;
      break;

  }
}

void adcSetNrfSaadc(uint32_t gain, uint32_t ref)
{
    saadcGain = gain;
    saadcReference = ref;
}

void adcSetReadResolution(int res)
{
  readResolution = res;
}

#endif

// code copied from https://github.com/espressif/arduino-esp32/blob/eb46978a8d96253309148c71cc10707f2721be5e/cores/esp32/esp32-hal-adc.c

bool IRAM_ATTR adcAttachPin(uint8_t pin)
{
    #ifdef ESP32
    int8_t channel = digitalPinToAnalogChannel(pin);
    if(channel < 0){
        return false;//not adc pin
    }

    int8_t pad = digitalPinToTouchChannel(pin);
    if(pad >= 0){
        uint32_t touch = READ_PERI_REG(SENS_SAR_TOUCH_ENABLE_REG);
        if(touch & (1 << pad)){
            touch &= ~((1 << (pad + SENS_TOUCH_PAD_OUTEN2_S))
                    | (1 << (pad + SENS_TOUCH_PAD_OUTEN1_S))
                    | (1 << (pad + SENS_TOUCH_PAD_WORKEN_S)));
            WRITE_PERI_REG(SENS_SAR_TOUCH_ENABLE_REG, touch);
        }
    } else if(pin == 25){
        CLEAR_PERI_REG_MASK(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_XPD_DAC | RTC_IO_PDAC1_DAC_XPD_FORCE);//stop dac1
    } else if(pin == 26){
        CLEAR_PERI_REG_MASK(RTC_IO_PAD_DAC2_REG, RTC_IO_PDAC2_XPD_DAC | RTC_IO_PDAC2_DAC_XPD_FORCE);//stop dac2
    }

    pinMode(pin, ANALOG);

    __analogInit();

    // low register access code below is actually from the original __analogInit
    // https://github.com/espressif/arduino-esp32/blob/8fb8478431a00cd46b2340c4dacb763688e79057/cores/esp32/esp32-hal-adc.c
    SET_PERI_REG_MASK(SENS_SAR_READ_CTRL_REG, SENS_SAR1_DATA_INV);
    SET_PERI_REG_MASK(SENS_SAR_READ_CTRL2_REG, SENS_SAR2_DATA_INV);

    SET_PERI_REG_MASK(SENS_SAR_MEAS_START1_REG, SENS_MEAS1_START_FORCE_M); // SAR ADC1 controller (in RTC) is started by SW
    SET_PERI_REG_MASK(SENS_SAR_MEAS_START1_REG, SENS_SAR1_EN_PAD_FORCE_M); // SAR ADC1 pad enable bitmap is controlled by SW
    SET_PERI_REG_MASK(SENS_SAR_MEAS_START2_REG, SENS_MEAS2_START_FORCE_M); // SAR ADC2 controller (in RTC) is started by SW
    SET_PERI_REG_MASK(SENS_SAR_MEAS_START2_REG, SENS_SAR2_EN_PAD_FORCE_M); // SAR ADC2 pad enable bitmap is controlled by SW

    CLEAR_PERI_REG_MASK(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR_M); // force XPD_SAR=0, use XPD_FSM
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_AMP, 0x2, SENS_FORCE_XPD_AMP_S); // force XPD_AMP=0

    CLEAR_PERI_REG_MASK(SENS_SAR_MEAS_CTRL_REG, 0xfff << SENS_AMP_RST_FB_FSM_S);  // clear FSM
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT1_REG, SENS_SAR_AMP_WAIT1, 0x1, SENS_SAR_AMP_WAIT1_S);
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT1_REG, SENS_SAR_AMP_WAIT2, 0x1, SENS_SAR_AMP_WAIT2_S);
    SET_PERI_REG_BITS(SENS_SAR_MEAS_WAIT2_REG, SENS_SAR_AMP_WAIT3, 0x1, SENS_SAR_AMP_WAIT3_S);
    while (GET_PERI_REG_BITS2(SENS_SAR_SLAVE_ADDR1_REG, 0x7, SENS_MEAS_STATUS_S) != 0); // wait det_fsm==
    #endif

    return true;
}

bool IRAM_ATTR adcStart(uint8_t pin)
{
    #ifdef ESP32
    int8_t channel = digitalPinToAnalogChannel(pin);
    if(channel < 0){
        return false; // not adc pin
    }

    if(channel > 7){
        channel -= 10;
        SET_PERI_REG_BITS(SENS_SAR_MEAS_START2_REG, SENS_SAR2_EN_PAD, (1 << channel), SENS_SAR2_EN_PAD_S);
        CLEAR_PERI_REG_MASK(SENS_SAR_MEAS_START2_REG, SENS_MEAS2_START_SAR_M);
        SET_PERI_REG_MASK(SENS_SAR_MEAS_START2_REG, SENS_MEAS2_START_SAR_M);
    } else {
        SET_PERI_REG_BITS(SENS_SAR_MEAS_START1_REG, SENS_SAR1_EN_PAD, (1 << channel), SENS_SAR1_EN_PAD_S);
        CLEAR_PERI_REG_MASK(SENS_SAR_MEAS_START1_REG, SENS_MEAS1_START_SAR_M);
        SET_PERI_REG_MASK(SENS_SAR_MEAS_START1_REG, SENS_MEAS1_START_SAR_M);
    }
    return true;
    #elif defined(NRF52840_XXAA) || defined(NRF52) || defined(NRF52_SERIES)
    uint32_t psel = SAADC_CH_PSELP_PSELP_NC;

    uint32_t ulPin = g_ADigitalPinMap[pin];

    switch ( ulPin ) {
      case 2:
        psel = SAADC_CH_PSELP_PSELP_AnalogInput0;
        break;
      case 3:
        psel = SAADC_CH_PSELP_PSELP_AnalogInput1;
        break;
      case 4:
        psel = SAADC_CH_PSELP_PSELP_AnalogInput2;
        break;
      case 5:
        psel = SAADC_CH_PSELP_PSELP_AnalogInput3;
        break;
      case 28:
        psel = SAADC_CH_PSELP_PSELP_AnalogInput4;
        break;
      case 29:
        psel = SAADC_CH_PSELP_PSELP_AnalogInput5;
        break;
      case 30:
        psel = SAADC_CH_PSELP_PSELP_AnalogInput6;
        break;
      case 31:
        psel = SAADC_CH_PSELP_PSELP_AnalogInput7;
        break;
      default:
        return false;
    }

    uint32_t saadcResolution;

    if (readResolution <= 8) {
        lastResolution = 8;
        saadcResolution = SAADC_RESOLUTION_VAL_8bit;
    } else if (readResolution <= 10) {
        lastResolution = 10;
        saadcResolution = SAADC_RESOLUTION_VAL_10bit;
    } else if (readResolution <= 12) {
        lastResolution = 12;
        saadcResolution = SAADC_RESOLUTION_VAL_12bit;
    } else {
        lastResolution = 14;
        saadcResolution = SAADC_RESOLUTION_VAL_14bit;
    }

    NRF_SAADC->RESOLUTION = saadcResolution;

    NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos);
    for (int i = 0; i < 8; i++) {
        NRF_SAADC->CH[i].PSELN = SAADC_CH_PSELP_PSELP_NC;
        NRF_SAADC->CH[i].PSELP = SAADC_CH_PSELP_PSELP_NC;
    }
    NRF_SAADC->CH[0].CONFIG =   ((SAADC_CH_CONFIG_RESP_Bypass   << SAADC_CH_CONFIG_RESP_Pos)   & SAADC_CH_CONFIG_RESP_Msk)
                              | ((SAADC_CH_CONFIG_RESP_Bypass   << SAADC_CH_CONFIG_RESN_Pos)   & SAADC_CH_CONFIG_RESN_Msk)
                              | ((saadcGain                     << SAADC_CH_CONFIG_GAIN_Pos)   & SAADC_CH_CONFIG_GAIN_Msk)
                              | ((saadcReference                << SAADC_CH_CONFIG_REFSEL_Pos) & SAADC_CH_CONFIG_REFSEL_Msk)
                              | ((saadcSampleTime               << SAADC_CH_CONFIG_TACQ_Pos)   & SAADC_CH_CONFIG_TACQ_Msk)
                              | ((SAADC_CH_CONFIG_MODE_SE       << SAADC_CH_CONFIG_MODE_Pos)   & SAADC_CH_CONFIG_MODE_Msk)
                              | ((saadcBurst                    << SAADC_CH_CONFIG_BURST_Pos)  & SAADC_CH_CONFIG_BURST_Msk);
    NRF_SAADC->CH[0].PSELN = psel;
    NRF_SAADC->CH[0].PSELP = psel;


    NRF_SAADC->RESULT.PTR = (uint32_t)&value_cache;
    NRF_SAADC->RESULT.MAXCNT = 1; // One sample

    NRF_SAADC->TASKS_START = 0x01UL;
    adc_state_machine = 1;
    return true;
    #endif
}

bool IRAM_ATTR adcBusy(uint8_t pin)
{
    #ifdef ESP32
    int8_t channel = digitalPinToAnalogChannel(pin);
    if(channel < 0){
        return false; // not adc pin
    }

    if(channel > 7){
        return (GET_PERI_REG_MASK(SENS_SAR_MEAS_START2_REG, SENS_MEAS2_DONE_SAR) == 0);
    }
    return (GET_PERI_REG_MASK(SENS_SAR_MEAS_START1_REG, SENS_MEAS1_DONE_SAR) == 0);
    #elif defined(NRF52840_XXAA) || defined(NRF52) || defined(NRF52_SERIES)
    if (adc_state_machine == 1) {
        if (NRF_SAADC->EVENTS_STARTED) {
            adc_state_machine = 2;
            NRF_SAADC->EVENTS_STARTED = 0;
            NRF_SAADC->TASKS_SAMPLE = 0x01UL;
        }
        return true;
    }
    else if (adc_state_machine == 2) {
        if (NRF_SAADC->EVENTS_END) {
            adc_state_machine = 3;
            NRF_SAADC->EVENTS_END = 0;
            NRF_SAADC->TASKS_STOP = 0x01UL;
        }
        return true;
    }
    else if (adc_state_machine == 3) {
        if (NRF_SAADC->EVENTS_STOPPED) {
            adc_state_machine = 4;
            NRF_SAADC->EVENTS_STOPPED = 0;
            NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos);
            return false;
        }
        return true;
    }
    return false;
    #endif
}

uint16_t IRAM_ATTR adcEnd(uint8_t pin)
{
    #ifdef ESP32
    uint16_t value = 0;
    int8_t channel = digitalPinToAnalogChannel(pin);
    if(channel < 0){
        return 0; // not adc pin
    }
    if(channel > 7){
        while (GET_PERI_REG_MASK(SENS_SAR_MEAS_START2_REG, SENS_MEAS2_DONE_SAR) == 0); // wait for conversion
        value = GET_PERI_REG_BITS2(SENS_SAR_MEAS_START2_REG, SENS_MEAS2_DATA_SAR, SENS_MEAS2_DATA_SAR_S);
    } else {
        while (GET_PERI_REG_MASK(SENS_SAR_MEAS_START1_REG, SENS_MEAS1_DONE_SAR) == 0); // wait for conversion
        value = GET_PERI_REG_BITS2(SENS_SAR_MEAS_START1_REG, SENS_MEAS1_DATA_SAR, SENS_MEAS1_DATA_SAR_S);
    }
    return value;
    #elif defined(NRF52840_XXAA) || defined(NRF52) || defined(NRF52_SERIES)
    if (adc_state_machine != 4) {
        if (adc_state_machine == 0) {
            adcStart(pin);
        }
        while (adcBusy(pin)) {
            yield();
        }
    }
    adc_state_machine = 0;
    return mapResolution(value_cache, lastResolution, readResolution);
    #endif
}
