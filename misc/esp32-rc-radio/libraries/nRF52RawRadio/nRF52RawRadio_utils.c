#include "nRF52RawRadio.h"

extern void RADIO_IRQHandler_cpp(void);

/**
 * @def NRF_802154_SWI_EGU_INSTANCE_NO
 *
 * Number of the SWI EGU instance used by the driver to synchronize PPIs and for requests and
 * notifications if SWI is in use.
 *
 */
#ifndef NRF_802154_SWI_EGU_INSTANCE_NO

#ifdef NRF52811_XXAA
#define NRF_802154_SWI_EGU_INSTANCE_NO 0
#else
#define NRF_802154_SWI_EGU_INSTANCE_NO 3
#endif

#endif // NRF_802154_SWI_EGU_INSTANCE_NO

/**
 * @def NRF_802154_SWI_EGU_INSTANCE
 *
 * The SWI EGU instance used by the driver to synchronize PPIs and for requests and notifications if
 * SWI is in use.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#define NRF_802154_SWI_EGU_INSTANCE NRFX_CONCAT_2(NRF_EGU, NRF_802154_SWI_EGU_INSTANCE_NO)

/**
 * @def NRF_802154_PPI_EGU_TO_RADIO_RAMP_UP
 *
 * The PPI channel that connects EGU event to RADIO_TXEN or RADIO_RXEN task.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_PPI_EGU_TO_RADIO_RAMP_UP
#define NRF_802154_PPI_EGU_TO_RADIO_RAMP_UP NRF_PPI_CHANNEL7
#endif

/**
 * @def NRF_802154_PPI_CORE_GROUP
 *
 * The PPI channel group used to disable self-disabling PPIs used by the core module.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_PPI_CORE_GROUP
#define NRF_802154_PPI_CORE_GROUP NRF_PPI_CHANNEL_GROUP0
#endif

/**
 * @def NRF_802154_PPI_RADIO_DISABLED_TO_EGU
 *
 * The PPI channel that connects RADIO_DISABLED event to EGU task.
 *
 * @note This option is used by the core module regardless of the driver configuration.
 *
 */
#ifndef NRF_802154_PPI_RADIO_DISABLED_TO_EGU
#define NRF_802154_PPI_RADIO_DISABLED_TO_EGU NRF_PPI_CHANNEL6
#endif

#define EGU_EVENT                  NRF_EGU_EVENT_TRIGGERED15
#define EGU_TASK                   NRF_EGU_TASK_TRIGGER15
#define PPI_CHGRP0                 NRF_802154_PPI_CORE_GROUP                     ///< PPI group used to disable self-disabling PPIs
#define PPI_CHGRP0_DIS_TASK        NRF_PPI_TASK_CHG0_DIS                         ///< PPI task used to disable self-disabling PPIs

#define PPI_DISABLED_EGU           NRF_802154_PPI_RADIO_DISABLED_TO_EGU          ///< PPI that connects RADIO DISABLED event with EGU task
#define PPI_EGU_RAMP_UP            NRF_802154_PPI_EGU_TO_RADIO_RAMP_UP           ///< PPI that connects EGU event with RADIO TXEN or RXEN task
#define PPI_EGU_TIMER_START        NRF_802154_PPI_EGU_TO_TIMER_START             ///< PPI that connects EGU event with TIMER START task
#define PPI_CRCERROR_CLEAR         NRF_802154_PPI_RADIO_CRCERROR_TO_TIMER_CLEAR  ///< PPI that connects RADIO CRCERROR event with TIMER CLEAR task
#define PPI_CCAIDLE_FEM            NRF_802154_PPI_RADIO_CCAIDLE_TO_FEM_GPIOTE    ///< PPI that connects RADIO CCAIDLE event with GPIOTE tasks used by FEM
#define PPI_TIMER_TX_ACK           NRF_802154_PPI_TIMER_COMPARE_TO_RADIO_TXEN    ///< PPI that connects TIMER COMPARE event with RADIO TXEN task
#define PPI_CRCOK_DIS_PPI          NRF_802154_PPI_RADIO_CRCOK_TO_PPI_GRP_DISABLE ///< PPI that connects RADIO CRCOK event with task that disables PPI group

#if NRF_802154_DISABLE_BCC_MATCHING
#define PPI_ADDRESS_COUNTER_COUNT  NRF_802154_PPI_RADIO_ADDR_TO_COUNTER_COUNT    ///< PPI that connects RADIO ADDRESS event with TIMER COUNT task
#define PPI_CRCERROR_COUNTER_CLEAR NRF_802154_PPI_RADIO_CRCERROR_COUNTER_CLEAR   ///< PPI that connects RADIO CRCERROR event with TIMER CLEAR task
#endif  // NRF_802154_DISABLE_BCC_MATCHING

#if NRF_802154_DISABLE_BCC_MATCHING
#define SHORT_ADDRESS_BCSTART 0UL
#else // NRF_802154_DISABLE_BCC_MATCHING
#define SHORT_ADDRESS_BCSTART NRF_RADIO_SHORT_ADDRESS_BCSTART_MASK
#endif  // NRF_802154_DISABLE_BCC_MATCHING

/// Value set to SHORTS register when no shorts should be enabled.
#define SHORTS_IDLE             0

/// Value set to SHORTS register for RX operation.
#define SHORTS_RX               (NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK | \
                                 NRF_RADIO_SHORT_END_DISABLE_MASK |       \
                                 SHORT_ADDRESS_BCSTART)

#define SHORTS_RX_FREE_BUFFER   (NRF_RADIO_SHORT_RXREADY_START_MASK)

#define SHORTS_TX_ACK           (NRF_RADIO_SHORT_TXREADY_START_MASK | \
                                 NRF_RADIO_SHORT_PHYEND_DISABLE_MASK)

#define SHORTS_CCA_TX           (NRF_RADIO_SHORT_RXREADY_CCASTART_MASK | \
                                 NRF_RADIO_SHORT_CCABUSY_DISABLE_MASK |  \
                                 NRF_RADIO_SHORT_CCAIDLE_TXEN_MASK |     \
                                 NRF_RADIO_SHORT_TXREADY_START_MASK |    \
                                 NRF_RADIO_SHORT_PHYEND_DISABLE_MASK)

#define SHORTS_TX               (NRF_RADIO_SHORT_TXREADY_START_MASK | \
                                 NRF_RADIO_SHORT_PHYEND_DISABLE_MASK)

#define SHORTS_RX_ACK           (NRF_RADIO_SHORT_ADDRESS_RSSISTART_MASK | \
                                 NRF_RADIO_SHORT_END_DISABLE_MASK)

#define SHORTS_ED               (NRF_RADIO_SHORT_READY_EDSTART_MASK)

#define SHORTS_CCA              (NRF_RADIO_SHORT_RXREADY_CCASTART_MASK | \
                                 NRF_RADIO_SHORT_CCABUSY_DISABLE_MASK)

/// Delay before first check of received frame: 24 bits is PHY header and MAC Frame Control field.
#define BCC_INIT                (3 * 8)

/// Duration of single iteration of Energy Detection procedure
#define ED_ITER_DURATION        128U
/// Overhead of hardware preparation for ED procedure (aTurnaroundTime) [number of iterations]
#define ED_ITERS_OVERHEAD       2U

#define CRC_LENGTH              2               ///< Length of CRC in 802.15.4 frames [bytes]
#define CRC_POLYNOMIAL          0x011021        ///< Polynomial used for CRC calculation in 802.15.4 frames

#define MHMU_MASK               0xff000700      ///< Mask of known bytes in ACK packet
#define MHMU_PATTERN            0x00000200      ///< Values of known bytes in ACK packet
#define MHMU_PATTERN_DSN_OFFSET 24              ///< Offset of DSN in MHMU_PATTER [bits]

#define ACK_IFS                 TURNAROUND_TIME ///< Ack Inter Frame Spacing [us] - delay between last symbol of received frame and first symbol of transmitted Ack
#define TXRU_TIME               40              ///< Transmitter ramp up time [us]
#define EVENT_LAT               23              ///< END event latency [us]

#define MAX_CRIT_SECT_TIME      60              ///< Maximal time that the driver spends in single critical section.

#define LQI_VALUE_FACTOR        4               ///< Factor needed to calculate LQI value based on data from RADIO peripheral
#define LQI_MAX                 0xff            ///< Maximal LQI value

/** Get LQI of given received packet. If CRC is calculated by hardware LQI is included instead of CRC
 *  in the frame. Length is stored in byte with index 0; CRC is 2 last bytes.
 */
#define RX_FRAME_LQI(data)      ((data)[(data)[0] - 1])

void nrfrr_ppis_for_egu_and_ramp_up_set(nrf_radio_task_t ramp_up_task, bool self_disabling);

/** Wait time needed to propagate event through PPI to EGU.
 *
 * During detection if trigger of DISABLED event caused start of hardware procedure, detecting
 * function needs to wait until event is propagated from RADIO through PPI to EGU. This delay is
 * required to make sure EGU event is set if hardware was prepared before DISABLED event was
 * triggered.
 */
static inline void ppi_and_egu_delay_wait(void)
{
    __ASM("nop");
    __ASM("nop");
    __ASM("nop");
    __ASM("nop");
    __ASM("nop");
    __ASM("nop");
}

/** Detect if PPI starting EGU for current operation worked.
 *
 * @retval  true   PPI worked.
 * @retval  false  PPI did not work. DISABLED task should be triggered.
 */
bool nrfrr_ppi_egu_worked(void)
{
    // Detect if PPIs were set before DISABLED event was notified. If not trigger DISABLE
    if (nrf_radio_state_get() != NRF_RADIO_STATE_DISABLED)
    {
        // If RADIO state is not DISABLED, it means that RADIO is still ramping down or already
        // started ramping up.
        return true;
    }

    // Wait for PPIs
    ppi_and_egu_delay_wait();

    if (nrf_egu_event_check(NRF_802154_SWI_EGU_INSTANCE, EGU_EVENT))
    {
        // If EGU event is set, procedure is running.
        return true;
    }
    else
    {
        return false;
    }
}

/** Initialize TX operation. */
bool nrfrr_tx_init(const uint8_t * p_data)
{
    uint32_t ints_to_enable = 0;

    nrf_radio_txpower_set(NRFRR_TXPOWER);
    nrf_radio_packetptr_set(p_data);

    // Set shorts
    nrf_radio_shorts_set(SHORTS_TX);

    // Enable IRQs

    nrf_radio_event_clear(NRF_RADIO_EVENT_END);
    ints_to_enable |= NRF_RADIO_INT_END_MASK;

    nrf_radio_int_enable(ints_to_enable);

    // Set FEM
    //fem_for_tx_set(cca);

    // Clr event EGU
    nrf_egu_event_clear(NRF_802154_SWI_EGU_INSTANCE, EGU_EVENT);

    // Set PPIs
    nrfrr_ppis_for_egu_and_ramp_up_set(NRF_RADIO_TASK_TXEN, true);

    if (!nrfrr_ppi_egu_worked())
    {
        nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
    }

    return true;
}

/** Set PPIs to connect DISABLED->EGU->RAMP_UP
 *
 * @param[in]  ramp_up_task    Task triggered to start ramp up procedure.
 * @param[in]  self_disabling  If PPI should disable itself.
 */
void nrfrr_ppis_for_egu_and_ramp_up_set(nrf_radio_task_t ramp_up_task, bool self_disabling)
{
    if (self_disabling)
    {
        nrf_ppi_channel_and_fork_endpoint_setup(PPI_EGU_RAMP_UP,
                                                (uint32_t)nrf_egu_event_address_get(
                                                    NRF_802154_SWI_EGU_INSTANCE,
                                                    EGU_EVENT),
                                                (uint32_t)nrf_radio_task_address_get(ramp_up_task),
                                                (uint32_t)nrf_ppi_task_address_get(
                                                    PPI_CHGRP0_DIS_TASK));
    }
    else
    {
        nrf_ppi_channel_endpoint_setup(PPI_EGU_RAMP_UP,
                                       (uint32_t)nrf_egu_event_address_get(
                                           NRF_802154_SWI_EGU_INSTANCE,
                                           EGU_EVENT),
                                       (uint32_t)nrf_radio_task_address_get(ramp_up_task));
    }

    nrf_ppi_channel_endpoint_setup(PPI_DISABLED_EGU,
                                   (uint32_t)nrf_radio_event_address_get(NRF_RADIO_EVENT_DISABLED),
                                   (uint32_t)nrf_egu_task_address_get(
                                       NRF_802154_SWI_EGU_INSTANCE,
                                       EGU_TASK));

    if (self_disabling)
    {
        nrf_ppi_channel_include_in_group(PPI_EGU_RAMP_UP, PPI_CHGRP0);
    }

    nrf_ppi_channel_enable(PPI_EGU_RAMP_UP);
    nrf_ppi_channel_enable(PPI_DISABLED_EGU);
}
