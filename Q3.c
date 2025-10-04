#include "sam.h"
#include "../display.h"

#define MS_TICKS        48000UL
#define BUTTON1_INDEX 2
#define BUTTON1_MASK PORT_PA02
#define BUTTON2_INDEX 7
#define BUTTON2_MASK PORT_PA07
#define FLASH_PARAMS_ADDR 0x000FE000
#define BUTTON_PRESSED(port_in, mask) (port_in & mask)
#define KEY2_HOLD_DOWN_TIME 1000
#define PIXEL2RATE 500
#define DOUBLE_CLICK 100
#define KEY_CLICK_LIMIT 1000
#define TWO_CLICK_TIME 100

typedef struct FLASH_PARAMS{
    uint8_t hyst_on_limit;
    uint8_t hyst_on_max;
    uint8_t hyst_off_limit;
    uint8_t hyst_off_min;
    uint8_t key1_source; // 0=SW0, 1=Click1
} flash_params;

typedef struct BUTTON {
  uint32_t last_clicked;
  uint32_t released_at;
  uint32_t click_time;
  uint32_t button;
  bool on;
  bool double_clicked;
  uint8_t pressCount;
  uint8_t releaseCount;
}button;

bool pixel1ON = false;
bool pixel2ON = false;
bool pixel2Flicker = false;
bool pixel2Flickering = false;
bool pixel3ON = false;
bool pixel4ON = true;
bool pixel4Handeled = false;
int pixel2RateCounter = 0;
volatile uint32_t msCount = 0;
volatile const flash_params *params = (flash_params*) FLASH_PARAMS_ADDR;

void heartInit() {
  NVIC_EnableIRQ(SysTick_IRQn);
  SysTick_Config(MS_TICKS);
}

void SysTick_Handler() {
  msCount++;
}

void button_setup(int index, int mask) {
  PORT_REGS->GROUP[0].PORT_DIRCLR = mask;
  PORT_REGS->GROUP[0].PORT_PINCFG[index] |= PORT_PINCFG_PMUXEN_Msk | PORT_PINCFG_PULLEN_Msk;
  PORT_REGS->GROUP[0].PORT_OUTSET = mask;
}

void initFromFlash() {
  uint8_t onLimit   = params->hyst_on_limit;
  uint8_t onMax     = params->hyst_on_max;
  uint8_t offLimit  = params->hyst_off_limit;
  uint8_t offMin    = params->hyst_off_min;
  uint8_t keyChoice = params->key1_source;
}

bool abs(uint32_t value1, uint32_t value2) {
  return (value1-value2 >= 0) ? value1-value2 : value2-value1;
}

void updateButton(uint32_t port_in, button *key) {
  if(BUTTON_PRESSED(port_in, key->button)) {
    key->pressCount++;
    key->releaseCount = 0;

    if(key->pressCount >= params->hyst_on_limit) {
      if(!key->on) {
        key->on = true;
        key->last_clicked = msCount;
        key->click_time = 0;
      }
    }
  } else {
    key->pressCount = 0;
    key->releaseCount++;

    if(key->releaseCount >= params->hyst_on_limit) {
      if(key->on) {
        key->on = false;
        key->click_time = msCount-key->last_clicked;
      }
    }
  }
}

void updateMainButton(uint32_t port_in, button *key) {
  if(!BUTTON_PRESSED(port_in, key->button)) {
    key->pressCount++;
    key->releaseCount = 0;

    if(key->pressCount >= params->hyst_on_limit) {
      if(!key->on) {
        key->on = true;
        key->last_clicked = msCount;
        key->click_time = 0;
        if(key->last_clicked-key->released_at < DOUBLE_CLICK) {
          key->double_clicked = true;
        }
      }
    }
  } else {
    key->pressCount = 0;
    key->releaseCount++;

    if(key->releaseCount >= params->hyst_on_limit) {
      if(key->on) {
        key->on = false;
        key->click_time = msCount-key->last_clicked;
        key->released_at = msCount;
      }
    }
  }
}

void pixel1(button *key) {
  if(!key->on){ // key is off

    pixel1ON = !pixel1ON;

    if(pixel1ON){
      displayDrawPixel(DISPLAY_SIZE/5, DISPLAY_SIZE/2, RED);
    }
    else{
      displayDrawPixel(DISPLAY_SIZE/5, DISPLAY_SIZE/2, BLACK);
    }
  }
}

void pixel2(button *key, bool flicker) {
  if(!key->on && !flicker) {// key is off and flicker is off

    pixel2ON = !pixel2ON;

    if(pixel2ON){
      displayDrawPixel(2*DISPLAY_SIZE/5, DISPLAY_SIZE/2, PINK);
    }
    else{
      displayDrawPixel(2*DISPLAY_SIZE/5, DISPLAY_SIZE/2, BLACK);
    }
  }
  if(flicker) {

    pixel2Flickering = !pixel2Flickering;

    if(pixel2Flickering){
      displayDrawPixel(2*DISPLAY_SIZE/5, DISPLAY_SIZE/2, PINK);
    }
    else{
      displayDrawPixel(2*DISPLAY_SIZE/5, DISPLAY_SIZE/2, BLACK);
    }
  }
}

void pixel3(button *key) {
  if(!key->on) {// key is off

    pixel3ON = !pixel3ON;

    if(pixel3ON){
      displayDrawPixel(3*DISPLAY_SIZE/5, DISPLAY_SIZE/2, ORANGE);
    }
    else{
      displayDrawPixel(3*DISPLAY_SIZE/5, DISPLAY_SIZE/2, BLACK);
    }
  }
}

void pixel4() {
  if(pixel4ON){
    displayDrawPixel(4*DISPLAY_SIZE/5, DISPLAY_SIZE/2, BLUE);
  }
  else{
    displayDrawPixel(4*DISPLAY_SIZE/5, DISPLAY_SIZE/2, BLACK);
  }
}

void logic(button *key1, button *key2) {
    if((abs(key1->last_clicked, key2->last_clicked) < TWO_CLICK_TIME && // key2 must be pressed no later than TWO_CLICK_TIME ms or its not a click at the same time
    key1->released_at-key2->last_clicked > DOUBLE_CLICK) || // key1 should be released atleast DOUBLE_CLICK ms after key2 clicked
    //OR
    (abs(key2->last_clicked, key1->last_clicked) < TWO_CLICK_TIME && // key1 must be pressed no later than TWO_CLICK_TIME ms or its not a click at the same time
    key2->released_at-key1->last_clicked > DOUBLE_CLICK)) { // key2 should be released atleast DOUBLE_CLICK ms after key1 clicked

    if(key1->on && key2->on && !pixel4Handeled) {
      pixel4();
      pixel4ON = !pixel4ON;
      pixel4Handeled = !pixel4Handeled;
    } else if(!key1->on && !key2->on && pixel4Handeled) {
      pixel4Handeled = !pixel4Handeled;
      key1->last_clicked = 0;
      key1->released_at = 0;
      key1->click_time = 0;
      key2->last_clicked = 0;
      key2->released_at = 0;
      key2->click_time = 0;
    }
  }

  if(!key1->on && !pixel4Handeled) {
    if(key1->double_clicked && key1->click_time > 0) {
      pixel3(key1);
      key1->click_time = 0;
      key1->double_clicked = false;
    }
    else if(key1->click_time < KEY_CLICK_LIMIT && key1->click_time > DOUBLE_CLICK) {
        pixel1(key1);
        key1->click_time = 0;
    }
    else if (key1->click_time >= KEY_CLICK_LIMIT*2) {
      displayErase();

      pixel1ON = false;
      pixel2ON = false;
      pixel3ON = false;
      pixel4ON = false;
      pixel2Flicker = false;
      pixel2RateCounter = 0;

      key1->click_time = 0;
      key2->click_time = 0;
    }
  }

  if(!key2->on && !pixel4Handeled) {
    if(key2->click_time < KEY_CLICK_LIMIT && key2->click_time > DOUBLE_CLICK) {
        pixel2(key2, false);
        key2->click_time = 0;
        pixel2Flicker = false;
    }
    if(key2->click_time > KEY2_HOLD_DOWN_TIME) {
      if(pixel2Flicker){// if the pixel is on after turning it off then this will turn the pixel off
        pixel2Flickering = true;
        pixel2(key2, true);
      }

      pixel2Flicker = !pixel2Flicker;
      pixel2RateCounter = 0;
      key2->click_time = 0;
      pixel2ON = false;
    }
  }
  if(pixel2Flicker){
    if(pixel2RateCounter%PIXEL2RATE == 0) {
      pixel2(key2, true);
      pixel2RateCounter = 0;
    }
    pixel2RateCounter++;
  }
}

int main(void)
{
  PORT_REGS->GROUP[0].PORT_DIRSET = PORT_PA14;
  PORT_REGS->GROUP[0].PORT_OUTSET = PORT_PA14;
  
  heartInit();
  button_setup(BUTTON1_INDEX, BUTTON1_MASK);
  button_setup(BUTTON2_INDEX, BUTTON2_MASK);
  button_setup(15, PORT_PA15);
  initFromFlash();
  displayInit();

  __enable_irq();

  button key1 = {0,0,0,(params->key1_source == 0) ? PORT_PA15 : BUTTON1_MASK,false,false,0,0};// 0 means the main button will be used otherwise button one will be used
  button key2 = {0,0,0,BUTTON2_MASK,false,false,0,0};

  while (1) 
  {
    __WFI();

    if(params->key1_source == 0) {
      updateMainButton(PORT_REGS->GROUP[0].PORT_IN, &key1);
    } else {
      updateButton(PORT_REGS->GROUP[0].PORT_IN, &key1);
    }
    updateButton(PORT_REGS->GROUP[0].PORT_IN, &key2);
    logic(&key1,&key2);
  }
}