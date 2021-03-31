#undef HID_ENABLED // Disable HID

#include "DueTimer.h"
#include <cmath>

// Arduino Due ADC->DMA->USB 1MSPS
// by stimmer
// from http://forum.arduino.cc/index.php?topic=137635.msg1136315#msg1136315
// Input: Analog in A0
// Output: Raw stream of uint16_t in range 0-4095 on Native USB Serial/ACM

// on linux, to stop the OS cooking your data: 
// stty -F /dev/ttyACM0 raw -iexten -echo -echoe -echok -echoctl -echoke -onlcr

volatile int bufn,obufn;
uint16_t buf[4][256];   // 4 buffers of 256 readings

uint16_t uVal  = 0; // New val for screen waiting to be updated

void ADC_Handler() {     // Move DMA pointers to next buffer
  int f = ADC->ADC_ISR;
  if (f & (1 << 27)){
    bufn = (bufn + 1) & 3;
    ADC->ADC_RNPR = (uint32_t)buf[bufn];
    ADC->ADC_RNCR = 256;
  } 
}

void setup() {
  // ------
  // Init I/O
  initDisp();
  hbInit();
  
  pinMode(LED_BUILTIN, OUTPUT); // "Heartbeat" LED
  
  // Init and wait for SerialUSB to connect
  SerialUSB.begin(0);

  // ------

  // DMA -> ADC
  pmc_enable_periph_clk(ID_ADC);
  adc_init(ADC, SystemCoreClock, ADC_FREQ_MAX, ADC_STARTUP_FAST);
  ADC->ADC_MR |=0x80; // free running

  ADC->ADC_CHER=0x80; 

  NVIC_EnableIRQ(ADC_IRQn);
  ADC -> ADC_IDR = ~(1 << 27);
  ADC -> ADC_IER = 1 << 27;
  ADC -> ADC_RPR = (uint32_t)buf[0];   // DMA buffer
  ADC -> ADC_RCR = 256;
  ADC -> ADC_RNPR = (uint32_t)buf[1]; // next DMA buffer
  ADC -> ADC_RNCR = 256;
  bufn = obufn = 1;
  ADC -> ADC_PTCR = 1;
  ADC -> ADC_CR = 2;
}

void loop() {
  while (obufn == bufn); // wait for buffer to be full
  // Send it - 512 bytes = 256 uint16_t
  SerialUSB.write((uint8_t *)buf[obufn], 512);
  // Totally ignore this
  obufn = (obufn + 1) & 3;
  uVal = buf[3][254] / 1.24121212; // PReCiSiON
  // SerialUSB.println(buf[3][254]);
}