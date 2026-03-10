// Temporary reference extract for the MCU -> FPGA SPI link bring-up.
// This file is intentionally standalone and is not used by the current build
// unless it is added explicitly to the MCU project.

#include <stdbool.h>
#include <stdint.h>

#include "mcu_definition.h"

// USIC0_CH1 register view used by the FPGA SPI path.
#define CH1_KSCFG          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x0CUL))
#define CH1_FDR            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x010UL))
#define CH1_CCR            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x040UL))
#define CH1_BRG            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x014UL))
#define CH1_SCTR           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x034UL))
#define CH1_PCR            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x03CUL))
#define CH1_PSR            (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x048UL))
#define CH1_PSCR           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x04CUL))
#define CH1_TBUF           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x080UL))
#define CH1_RBUF           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x054UL))
#define CH1_TCSR           (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x038UL))
#define CH1_DX0CR          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x01CUL))
#define CH1_DX1CR          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x020UL))
#define CH1_DX2CR          (*(volatile uint32_t*)(USIC0_CH1_BASE + 0x024UL))

#define GPIO_MODE_INPUT_TRISTATE (0x00U)
#define GPIO_MODE_PP_GPIO        (0x10U)
#define GPIO_MODE_PP_ALT7        (0x17U)

#define P0_0_IOCR_SHIFT        (3U)
#define P0_6_IOCR_SHIFT        (19U)
#define P0_7_IOCR_SHIFT        (27U)
#define P0_8_IOCR_SHIFT        (3U)

#define FPGA_CS_MASK           (1U << 0)

#define USIC_CCR_MODE_DISABLED (0x0U)
#define USIC_CCR_MODE_SSC      (0x1U)

#define USIC_SCTR_SDIR_MSB     (1U << 0)
#define USIC_SCTR_PDL_MASK     (1U << 1)
#define USIC_SCTR_TRM_STD      (1U << 8)
#define USIC_SCTR_FLE(v)       ((uint32_t)(v) << 16)
#define USIC_SCTR_WLE(v)       ((uint32_t)(v) << 24)

#define USIC_FDR_DM_DIV        (0x1U << 14)
#define USIC_FDR_STEP_MAX      (0x3FFU)

#define USIC_BRG_CLKSEL_DX1T   (0U << 0)
#define USIC_BRG_DCTQ(v)       ((uint32_t)(v) << 10)
#define USIC_BRG_PDIV(v)       ((uint32_t)(v) << 16)
#define USIC_BRG_SCLKCFG(v)    ((uint32_t)(v) << 30)

#define USIC_DXCR_DSEL_C       (2U << 0)
#define USIC_DXCR_DSEL_HIGH    (7U << 0)
#define USIC_DXCR_INSW_DIRECT  (0U << 4)
#define USIC_DXCR_INSW_INPUT   (1U << 4)

#define USIC_KSCFG_MODEN       (1U << 0)
#define USIC_KSCFG_BPMODEN     (1U << 1)

#define USIC_TCSR_TDSSM        (1U << 8)
#define USIC_TCSR_TDEN(v)      ((uint32_t)(v) << 10)

#define USIC_PCR_SSC_MSLSEN    (1U << 0)
#define USIC_PCR_SSC_SLPHSEL   (1U << 25)

#define PSR_TSIF_MASK          (1U << 12)
#define USIC_WAIT_TIMEOUT      (200000UL)

static bool TEMP_SPI_FPGA_WaitForShiftDone(void)
{
    uint32_t timeout = USIC_WAIT_TIMEOUT;

    while (timeout-- != 0U) {
        if ((CH1_PSR & PSR_TSIF_MASK) != 0U) {
            return true;
        }
    }

    return false;
}

void TEMP_SPI_FPGA_ClockEnable(void)
{
    // Same SCU protection sequence used by the main firmware.
    SCU_PASSWD = 0x000000C0UL;
    SCU_CGATCLR0 = (1U << 3);
    SCU_PASSWD = 0x000000C3UL;
}

void TEMP_SPI_FPGA_Init(void)
{
    // Enable the USIC kernel before touching channel 1.
    TEMP_SPI_FPGA_ClockEnable();

    // 1. Disable channel and clear status.
    CH1_KSCFG &= ~(USIC_KSCFG_MODEN | USIC_KSCFG_BPMODEN);
    CH1_CCR = USIC_CCR_MODE_DISABLED;
    CH1_PSCR = 0xFFFFFFFFU;

    // 2. Configure only the pins used by the FPGA link.
    // P0.6 -> MISO input
    // P0.7 -> MOSI (ALT7)
    // P0.8 -> SCLKOUT (ALT7)
    // P0.0 -> manual FPGA chip-select
    P0_IOCR4 &= ~(0x1FUL << P0_6_IOCR_SHIFT);
    P0_IOCR4 |= (GPIO_MODE_INPUT_TRISTATE << P0_6_IOCR_SHIFT);

    P0_IOCR4 &= ~(0x1FUL << P0_7_IOCR_SHIFT);
    P0_IOCR4 |= (GPIO_MODE_PP_ALT7 << P0_7_IOCR_SHIFT);

    P0_IOCR8 &= ~(0x1FUL << P0_8_IOCR_SHIFT);
    P0_IOCR8 |= (GPIO_MODE_PP_ALT7 << P0_8_IOCR_SHIFT);

    P0_IOCR0 &= ~(0x1FUL << P0_0_IOCR_SHIFT);
    P0_IOCR0 |= (GPIO_MODE_PP_GPIO << P0_0_IOCR_SHIFT);

    // Keep the FPGA deselected while the peripheral comes up.
    P0_OMR = FPGA_CS_MASK;

    // 3. Route SSC inputs and select a conservative SPI clock.
    CH1_DX0CR = USIC_DXCR_INSW_INPUT  | USIC_DXCR_DSEL_C;
    CH1_DX1CR = USIC_DXCR_INSW_DIRECT;
    CH1_DX2CR = USIC_DXCR_INSW_DIRECT | USIC_DXCR_DSEL_HIGH;

    CH1_FDR = USIC_FDR_DM_DIV | USIC_FDR_STEP_MAX;
    CH1_BRG = USIC_BRG_CLKSEL_DX1T |
              USIC_BRG_DCTQ(1U) |
              USIC_BRG_PDIV(63U) |
              USIC_BRG_SCLKCFG(0U);

    // 4. Configure 16-bit, MSB-first SSC transfers.
    CH1_SCTR = USIC_SCTR_SDIR_MSB |
               USIC_SCTR_TRM_STD |
               USIC_SCTR_FLE(15U) |
               USIC_SCTR_WLE(15U);
    CH1_SCTR &= ~USIC_SCTR_PDL_MASK;

    CH1_TCSR = USIC_TCSR_TDSSM | USIC_TCSR_TDEN(3U);
    CH1_PCR  = USIC_PCR_SSC_MSLSEN;
    CH1_PCR &= ~USIC_PCR_SSC_SLPHSEL;
    CH1_CCR  = USIC_CCR_MODE_SSC;

    // 5. Enable channel.
    CH1_KSCFG |= (USIC_KSCFG_MODEN | USIC_KSCFG_BPMODEN);
    (void)CH1_KSCFG;
}

bool TEMP_SPI_FPGA_SendWord(uint16_t data)
{
    // Assert manual chip-select.
    P0_OMR = (FPGA_CS_MASK << 16);

    CH1_PSCR = PSR_TSIF_MASK;
    CH1_TBUF = data;

    if (!TEMP_SPI_FPGA_WaitForShiftDone()) {
        P0_OMR = FPGA_CS_MASK;
        return false;
    }

    CH1_PSCR = PSR_TSIF_MASK;

    // Release chip-select and drain the receive side.
    P0_OMR = FPGA_CS_MASK;
    (void)CH1_RBUF;
    return true;
}
