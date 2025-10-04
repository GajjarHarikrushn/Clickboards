#include "sam.h"
#include "../display.h"

#define MS_TICKS        48000UL
#define BUTTON1_INDEX 2
#define BUTTON1_MASK PORT_PA02
#define BUTTON2_INDEX 7
#define BUTTON2_MASK PORT_PA07
#define BUTTON3_INDEX 18
#define BUTTON3_MASK PORT_PA18
#define BUTTON4_INDEX 10
#define BUTTON4_MASK PORT_PA10
#define FLASH_PARAMS_ADDR 0x000FE000
#define BUTTON_PRESSED(port_in, mask) (!(port_in & mask))

typedef struct BUTTON {
  uint32_t last_clicked;
  uint32_t click_time;
  uint32_t button;
  bool on;
}button;

volatile uint32_t msCount = 0;

void heartInit() {
  NVIC_EnableIRQ(SysTick_IRQn);
  SysTick_Config(MS_TICKS);
}

void SysTick_Handler() {
  msCount++;
}

void BUTTON_SETUP(int index, int mask) {
  PORT_REGS->GROUP[0].PORT_DIRCLR = mask;
  PORT_REGS->GROUP[0].PORT_PINCFG[index] |= PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_PULLEN_Msk;
  PORT_REGS->GROUP[0].PORT_OUTSET = mask;
}


int main(void)
{
  PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;
  
  heartInit();
  BUTTON_SETUP(BUTTON1_INDEX, BUTTON1_MASK);
  displayInit();

  __enable_irq();

  button key2 = {0,0,BUTTON1_MASK, false};

  while (1) 
  {
    __WFI();
    int port_in = PORT_REGS->GROUP[0].PORT_IN;
    if(port_in & BUTTON1_MASK) {
        displayDrawPixel(0,0,WHITE);
    } else {
        displayDrawPixel(0,0,BLACK);
    }
  }
}