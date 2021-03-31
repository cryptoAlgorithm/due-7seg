// Display config
#define DISP_DIGITS 4

// Pins
const uint8_t disp_seg_pins[] = { 22, 25, 26, 29, 30, 33, 34, 37 };
const uint8_t disp_dig_pins[] = { 50, 51, 52, 53 };

/* ###### Display maps ###### */
const unsigned char seven_seg_digits_decode_abcdefg[75]= {
/*  0     1     2     3     4     5     6     7     8     9     :     ;     */
    0x7E, 0x30, 0x6D, 0x79, 0x33, 0x5B, 0x5F, 0x70, 0x7F, 0x7B, 0x00, 0x00, 
/*  <     =     >     ?     @     A     B     C     D     E     F     G     */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x77, 0x1F, 0x4E, 0x3D, 0x4F, 0x47, 0x5E, 
/*  H     I     J     K     L     M     N     O     P     Q     R     S     */
    0x37, 0x06, 0x3C, 0x57, 0x0E, 0x55, 0x15, 0x1D, 0x67, 0x73, 0x05, 0x5B, 
/*  T     U     V     W     X     Y     Z     [     \     ]     ^     _     */
    0x0F, 0x3E, 0x1C, 0x5C, 0x13, 0x3B, 0x6D, 0x00, 0x00, 0x00, 0x00, 0x00, 
/*  `     a     b     c     d     e     f     g     h     i     j     k     */
    0x00, 0x77, 0x1F, 0x4E, 0x3D, 0x4F, 0x47, 0x5E, 0x37, 0x06, 0x3C, 0x57, 
/*  l     m     n     o     p     q     r     s     t     u     v     w     */
    0x0E, 0x55, 0x15, 0x1D, 0x67, 0x73, 0x05, 0x5B, 0x0F, 0x3E, 0x1C, 0x5C, 
/*  x     y     z     */
    0x13, 0x3B, 0x6D
};
/* ############ */

// Display vars
uint32_t digit = 0;
uint16_t cVal  = 0; // Current val to be displayed on screen
uint16_t uCyc  = 0;
uint16_t sBuf[] = {0x00, 0x00, 0x00, 0x00};
bool actLEDSt  = true;

unsigned char decode_7seg(char chr) {
  if (chr > (unsigned char)'z') return 0x00;
  return seven_seg_digits_decode_abcdefg[chr - '0'];
}

void initDisp() {
  pinMode(72, OUTPUT); // Activity LED
  
  for (int i = 0; i < 8; i++) {
    pinMode(disp_seg_pins[i], OUTPUT);
  }
  for (int i = 0; i < DISP_DIGITS; i++) {
    pinMode(disp_dig_pins[i], OUTPUT);
  }

  // Start h/w timer
  Timer6.attachInterrupt(screenUpdate).start(5000); // Around 50 refreshes / sec
}

char getDigit(uint16_t value, byte digit) {
  return static_cast<uint16_t>(value / pow(10, digit)) % 10;
}

void writeDigit(char pins, bool dp) {
  for (int i = 0; i < 7; i++) {
    digitalWrite(disp_seg_pins[i], (pins >> (6 - i)) & 1);
  }
  digitalWrite(disp_seg_pins[7], dp);
}

void updateScnBuff(uint16_t value) {
  for (int i = 0; i < DISP_DIGITS; i++) {
    
  }
}

void screenUpdate() {
  for (int i = 0; i < DISP_DIGITS; i++) {
    digitalWrite(disp_dig_pins[i], digit != i);
  }

  const bool dp = (digit == 0);

  SerialUSB.print(digit);

  writeDigit(decode_7seg(getDigit(cVal, DISP_DIGITS - 1 - digit) + '0'), dp);
  
  digit++;
  uCyc ++;

  if (uCyc == DISP_DIGITS * 5) {
    // Update digits
    cVal = uVal;
    uCyc = 0;
    
    // Toggle heartbeat LED
    digitalWrite(72, actLEDSt);
    actLEDSt = !actLEDSt;
  }

  if (digit >= DISP_DIGITS) digit = 0;
}
