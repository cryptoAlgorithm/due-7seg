#undef HID_ENABLED // Disable HID

// Screen settings
#define REF_RATE 500
#define REF_INT  1000000 / REF_RATE
#define DISP_DIGITS 4

// ADC/DMA settings
#define BUF_PRI_LEN 8192

// IO pins
#define OK_BTN_PIN 44

#include "DueTimer.h"
#include <cmath>

// Arduino Due ADC -> DMA -> USB 1MSPS
// Input: Analog in A0
// Output: Raw stream of uint16_t in range 0-4095 on Native USB Serial/ACM

// on linux, to stop the OS cooking your data: 
// stty -F /dev/ttyACM0 raw -iexten -echo -echoe -echok -echoctl -echoke -onlcr

volatile int bufn, obufn;
uint16_t buf[4][BUF_PRI_LEN];    // 4 buffers of 256 readings

void ADC_Handler() {     // Move DMA pointers to next buffer
  int f = ADC -> ADC_ISR;
  if (f & (1 << 27)){
    bufn = (bufn + 1) & 3;
    ADC -> ADC_RNPR = (uint32_t)buf[bufn];
    ADC -> ADC_RNCR = BUF_PRI_LEN;
  } 
}

void setup() {
  // ------
  // Init I/O
  pinMode(OK_BTN_PIN, INPUT_PULLUP);
  
  hbInit();
  initDisp(REF_INT);

  attachInterrupt(digitalPinToInterrupt(OK_BTN_PIN), toggleDispSleep, RISING); // Set button interrupt
  
  // Init and wait for SerialUSB to connect
  SerialUSB.begin(0);

  // ------

  // DMA -> ADC
  pmc_enable_periph_clk(ID_ADC);
  adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
  ADC -> ADC_MR  |= 0x80; // ADC free running mode

  ADC -> ADC_CHER = 0x80; 

  NVIC_EnableIRQ(ADC_IRQn);
  ADC -> ADC_IDR = ~(1 << 27);
  ADC -> ADC_IER = 1 << 27;
  ADC -> ADC_RPR = (uint32_t)buf[0];  // DMA buffer
  ADC -> ADC_RCR = BUF_PRI_LEN;
  ADC -> ADC_RNPR = (uint32_t)buf[1]; // Next DMA buffer
  ADC -> ADC_RNCR = BUF_PRI_LEN;
  bufn = obufn = 1;
  ADC -> ADC_PTCR = 1;
  ADC -> ADC_CR = 2;
}

void loop() {
  while (obufn == bufn); // wait for buffer to be full
  // Send it - 512 bytes = 256 uint16_t
  SerialUSB.write((uint8_t *)buf[obufn], BUF_PRI_LEN * 2);
  // Totally ignore this
  obufn = (obufn + 1) & 3;
  updateDataBuff(buf[3][BUF_PRI_LEN - 1] / 1.24121212); // PReCiSiON
  // SerialUSB.println(buf[3][254]);
}
