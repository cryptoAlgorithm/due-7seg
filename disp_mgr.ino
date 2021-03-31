// Display config
#define DISP_DIGITS 4

// Pins
const uint8_t disp_seg_pins[] PROGMEM = { 22, 25, 26, 29, 30, 33, 34, 37 };
const uint8_t disp_dig_pins[] PROGMEM = { 50, 51, 52, 53 };

/* ###### Display maps ###### */
const unsigned char seven_seg_digits_decode_abcdefg[75] PROGMEM = {
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
uint16_t uCyc  = 0;
uint16_t uVal  = 0; // New val for screen waiting to be updated
uint8_t  scnOp = 0; // 0 - Update values; 1 - Show scrolling text; 2 - Show status
String   tBuf  = String();
unsigned char sBuf[] = {0x00, 0x00, 0x00, 0x00};
bool actLEDSt  = true;

unsigned char decode_7seg(unsigned char chr) {
  if (chr > (unsigned char)'z' || chr < (unsigned char)'0') return 0x00;
  return seven_seg_digits_decode_abcdefg[chr - '0'];
}

void updateDataBuff(uint16_t value) {
  uVal = value;
}

void initDisp(uint16_t ref_rate) {
  pinMode(72, OUTPUT); // Activity LED
  
  for (int i = 0; i < 8; i++) pinMode(disp_seg_pins[i], OUTPUT);
  for (int i = 0; i < DISP_DIGITS; i++) pinMode(disp_dig_pins[i], OUTPUT);

  // Start h/w timer
  Timer6.attachInterrupt(screenUpdate).start(ref_rate); // Around 50 refreshes / sec

  // Show init sequence
  tBuf = "Hello there";
  scnOp = 1;
  delay(7800);
  scnOp = 0;
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

void updateScnBuffWithData(uint16_t value) {
  for (int i = 0; i < DISP_DIGITS; i++) sBuf[i] = decode_7seg(getDigit(value, DISP_DIGITS - 1 - i) + '0');
}

uint16_t txtScrollLoc = 0;
uint8_t  txtScrollInt = 0;
void updateScnBuffWithText(String txt) {
  String buff = txt.substring(txtScrollLoc, txtScrollLoc + 4);
  for (int i = 0; i < DISP_DIGITS; i++) sBuf[i] = decode_7seg(buff[i]);
  
  txtScrollInt++;
  if (txtScrollInt == 5) {
    txtScrollInt = 0;
    txtScrollLoc++;
    if (txtScrollLoc + 3 >= txt.length()) txtScrollLoc = 0;
  }
}

void screenUpdate() {
  for (int i = 0; i < DISP_DIGITS; i++) digitalWrite(disp_dig_pins[i], digit != i);

  writeDigit(sBuf[digit], digit == 0 && scnOp == 0);
  
  digit++;
  uCyc ++;

  if (uCyc == REF_RATE / 5) {
    // Update digits
    switch (scnOp) {
      case 0: updateScnBuffWithData(uVal); actLEDSt = !actLEDSt; break;
      case 1: updateScnBuffWithText(tBuf); break;
    }
    
    // Toggle heartbeat LED
    digitalWrite(72, actLEDSt);

    uCyc = 0;
  }

  if (digit >= DISP_DIGITS) digit = 0;
}
