#include "sam.h"
#include "../display.h"

volatile int x = 0;
volatile int numOfOverflow = 0;
volatile int timeValue = 0;
int ticks[] = {48000, (48000*5)%65536, (48000*10)%65536, (48000*50)%65536, (48000*100)%65536};
int overflow[] = {1, (48000*5)/65536, (48000*10)/65536, (48000*50)/65536, (48000*100)/65536};
typedef void (*TimeFunc) (void);

void TC0_Handler() {
    if (timeValue >= 1){
        if (numOfOverflow == overflow[timeValue]){
            TC0_REGS->COUNT16.TC_COUNT = 65536-ticks[timeValue];
        }
        if (numOfOverflow > overflow[timeValue]){
            displayReplacePixel(x,DISPLAY_SIZE/2,WHITE);
            x = (x+1)%DISPLAY_SIZE;
            numOfOverflow = -1;
        }
        numOfOverflow++;
    } else {
        displayReplacePixel(x,DISPLAY_SIZE/2,WHITE);
        x = (x+1)%DISPLAY_SIZE;
        TC0_REGS->COUNT16.TC_COUNT = 65536-ticks[timeValue];
    }
    TC0_REGS->COUNT16.TC_INTFLAG = TC_INTFLAG_OVF_Msk;
}

void TC0_ENABLE() {
    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_TC0_Msk;
    GCLK_REGS->GCLK_PCHCTRL[TC0_GCLK_ID] = GCLK_PCHCTRL_CHEN_Msk;

    TC0_REGS->COUNT16.TC_INTENSET = TC_INTENSET_OVF_Msk;

    NVIC_EnableIRQ(TC0_IRQn);

    TC0_REGS->COUNT16.TC_CTRLA = TC_CTRLA_ENABLE_Msk | TC_CTRLA_CAPTEN0_Msk;
    TC0_REGS->COUNT16.TC_COUNT = 65536-48000;
}

void EIC_EXTINT_15_Handler() {
    EIC_REGS->EIC_INTFLAG = PORT_PA15;
    timeValue = (timeValue+1)%5;
}

void EIC_ENABLE() {
    MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_EIC_Msk;

    PORT_REGS->GROUP[0].PORT_DIRCLR = PORT_PA15;
    PORT_REGS->GROUP[0].PORT_PINCFG[15] = PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_PULLEN_Msk;
    PORT_REGS->GROUP[0].PORT_PMUX[7] = PORT_PMUX_PMUXO_A;
    PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA15;

    NVIC_EnableIRQ(EIC_EXTINT_15_IRQn);

    EIC_REGS->EIC_INTENSET = PORT_PA15;
    EIC_REGS->EIC_INTFLAG = PORT_PA15;
    EIC_REGS->EIC_CONFIG[1] = EIC_CONFIG_SENSE7_RISE;
    EIC_REGS->EIC_CTRLA = EIC_CTRLA_ENABLE_Msk | EIC_CTRLA_CKSEL_CLK_ULP32K;
}

int main() {
    displayInit();
    // TC0_ENABLE();
    EIC_ENABLE();
    __enable_irq();
    while(1){
        __WFI();
    }
}