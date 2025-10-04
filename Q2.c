#include "sam.h"
#include "../display.h"

#define MS_TICKS        48000UL
#define DIGIT_WIDTH     8
#define DIGIT_LENGTH    12
#define CENTER          DISPLAY_SIZE/2

#define LED_FLASH_MS    250
#define SCREEN_REFRESH  500

volatile uint32_t msCount = 0;
volatile uint32_t time = 0;
volatile uint32_t elapsed_time = 0;
volatile uint8_t int_fired = 0;
static const int pow10[] = {1, 10, 100, 1000, 10000, 100000};

void displayClock(uint32_t time) {
  displayDrawPixel(CENTER,CENTER+3,WHITE);// the dot between the clock
  displayDrawPixel(CENTER,CENTER-3,WHITE);// the dot between the clock

  for(int i = 1; i <= 3; i++) {// draw seconds
    displayDrawDigit(CENTER-(DIGIT_LENGTH/2),CENTER-(DIGIT_LENGTH*i),WHITE,(time/pow10[i+2])%10);
  }
  
  for(int i = 1; i <= 3; i++) {// draw milliseconds
    displayDrawDigit(CENTER-(DIGIT_LENGTH/2),CENTER+((DIGIT_LENGTH*i-DIGIT_WIDTH)),WHITE,(time/pow10[3-i])%10);
  }
}

void heartInit() {
  NVIC_EnableIRQ(SysTick_IRQn);
  SysTick_Config(MS_TICKS);
}

void SysTick_Handler() {
  msCount++;
}

void TC0_Handler() {
  TC0_REGS->COUNT16.TC_COUNT += ((1<<16)-60000);
  int_fired++;
  if(int_fired == 2) {
    int_fired = 0;
    elapsed_time += 10;
  }
  TC0_REGS->COUNT16.TC_INTFLAG = TC_INTFLAG_OVF_Msk;
}

void TC0_ENABLE() {
  MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_TC0_Msk;
  GCLK_REGS->GCLK_PCHCTRL[TC0_GCLK_ID] = GCLK_PCHCTRL_CHEN_Msk;

  TC0_REGS->COUNT16.TC_INTENSET = TC_INTENSET_OVF_Msk;

  NVIC_EnableIRQ(TC0_IRQn);

  TC0_REGS->COUNT16.TC_CTRLA = TC_CTRLA_ENABLE_Msk | TC_CTRLA_CAPTEN0_Msk | TC_CTRLA_PRESCALER_DIV8;
  TC0_REGS->COUNT16.TC_COUNT += ((1<<16)-60000);
}

void EIC_EXTINT_15_Handler() {
  EIC_REGS->EIC_INTFLAG = PORT_PA15;
  time = elapsed_time;
}

void EIC_SETUP_15() {
  MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_EIC_Msk;

  PORT_REGS->GROUP[0].PORT_DIRCLR = PORT_PA15;
  PORT_REGS->GROUP[0].PORT_PINCFG[15] |= PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_PULLEN_Msk;
  PORT_REGS->GROUP[0].PORT_PMUX[7] = PORT_PMUX_PMUXO_A;
  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA15;

  NVIC_EnableIRQ(EIC_EXTINT_15_IRQn);

  EIC_REGS->EIC_INTENSET = PORT_PA15;
  EIC_REGS->EIC_INTFLAG = PORT_PA15;
  EIC_REGS->EIC_CONFIG[1] = EIC_CONFIG_SENSE7_RISE;
}

void EIC_EXTINT_4_Handler() {
  EIC_REGS->EIC_INTFLAG = PORT_PA04;
  elapsed_time = 0;
  time = 0;
}

void EIC_SETUP_4() {
  MCLK_REGS->MCLK_APBAMASK |= MCLK_APBAMASK_EIC_Msk;

  PORT_REGS->GROUP[0].PORT_DIRCLR = PORT_PA04;
  PORT_REGS->GROUP[0].PORT_PINCFG[4] |= PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_PULLEN_Msk;
  PORT_REGS->GROUP[0].PORT_PMUX[2] = PORT_PMUX_PMUXE_A;
  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA04;

  NVIC_EnableIRQ(EIC_EXTINT_4_IRQn);

  EIC_REGS->EIC_INTENSET = PORT_PA04;
  EIC_REGS->EIC_INTFLAG = PORT_PA04;
  EIC_REGS->EIC_CONFIG[0] |= EIC_CONFIG_SENSE4_RISE;
}

void EIC_ENABLE_CTRLA() {
  EIC_REGS->EIC_CTRLA = EIC_CTRLA_ENABLE_Msk | EIC_CTRLA_CKSEL_CLK_ULP32K;
}

int main(void)
{
  PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;
  
  heartInit();
  TC0_ENABLE();
  EIC_SETUP_15();
  EIC_SETUP_4();
  EIC_ENABLE_CTRLA();
  displayInit();

  displayClock(0);

  __enable_irq();

  while (1) 
  {
    __WFI();

    if ((msCount % LED_FLASH_MS) == 0)
    {
      PORT_REGS->GROUP[0].PORT_OUTTGL = PORT_PA14;
    }

    if ((msCount % SCREEN_REFRESH) == 0)
    {
      displayClock(time);
    }
  }
}