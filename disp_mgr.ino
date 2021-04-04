// Pins
const uint8_t disp_seg_pins[] PROGMEM = { 22, 25, 26, 29, 30, 33, 34, 37 };
const uint8_t disp_dig_pins[] PROGMEM = { 50, 51, 52, 53 };

/* ###### Display maps ###### */
const uint8_t seven_seg_digits_decode_abcdefg[75] PROGMEM = {
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
volatile int8_t scnOp  = 0; // -1 - Show scrolling text; 0 - Update values;
String   tBuf  = String();
uint8_t sBuf[] = {0x00, 0x00, 0x00, 0x00};
uint8_t errState = 0; // 0 = no error
bool actLEDSt  = true;
volatile long ignoreBtn = 0;

// Value setter functions

void updateDataBuff(uint16_t value) {
  uVal = value;
}

void changeMode() {
  if (millis() - 250 < ignoreBtn) return;
  // Do whatever you want to do here
  scnOp++;
  if (scnOp > MAX_MODE) scnOp = 0;
  
  ignoreBtn = millis();
}

uint8_t decode_7seg(unsigned char chr) {
  if (chr > (unsigned char)'z' || chr < (unsigned char)'0') return 0x00;
  return seven_seg_digits_decode_abcdefg[chr - '0'] << 1;
}

RTC_DS1307 rtc; // RTC object

void initDisp(uint16_t ref_rate) {
  pinMode(72, OUTPUT); // Activity LED
  
  for (int i = 0; i < 8; i++) pinMode(disp_seg_pins[i], OUTPUT);
  for (int i = 0; i < DISP_DIGITS; i++) pinMode(disp_dig_pins[i], OUTPUT);

  // Start h/w timer
  Timer6.attachInterrupt(screenUpdate).start(ref_rate); // Around 50 refreshes / sec

  // Init RTC
  if (!rtc.begin()) {
    errState = 10;
  }

  if (!rtc.isrunning()) {
    errState = 12;
  }
  
  // Show init sequence
  tBuf = "Hello there";
  scnOp = -1;
  delay(3350);
  scnOp = 0;

  ignoreBtn = millis();
}

char getDigit(uint16_t value, byte digit) {
  return static_cast<uint16_t>(value / pow(10, digit)) % 10;
}

void writeDigit(uint8_t pins) {
  for (int i = 0; i < 8; i++) digitalWrite(disp_seg_pins[i], (pins >> (7 - i)) & 1);
}

void updateScnBuffWithData(uint16_t value) {
  for (int i = 0; i < DISP_DIGITS; i++) sBuf[i] = decode_7seg(getDigit(value, DISP_DIGITS - 1 - i) + '0');
  sBuf[0] = sBuf[0] | 0b00000001; // Add decimal point dot
}

uint16_t txtScrollLoc = 0;
uint8_t  txtScrollInt = 0;
void updateScnBuffWithText(String txt) {
  String buff = txt.substring(txtScrollLoc, txtScrollLoc + 4);
  for (int i = 0; i < DISP_DIGITS; i++) sBuf[i] = decode_7seg(buff[i]);
  
  txtScrollInt++;
  if (txtScrollInt == 2) {
    txtScrollInt = 0;
    txtScrollLoc++;
    if (txtScrollLoc + 3 >= txt.length()) txtScrollLoc = 0;
  }
}

void dispSleep() {
  sBuf[0] = 0x00;
  sBuf[1] = 0x00;
  sBuf[2] = 0x00;
  sBuf[3] = 0x00;
}

void updateScnBuffWithTime() {
  // Update the disp time
  uint16_t timeBuff = 0;
  DateTime now = rtc.now();
  timeBuff += now.hour() * 100;
  timeBuff += now.minute();

  // Then do the same thing as updateScnBuffWithDate to populate display buff
  for (int i = 0; i < DISP_DIGITS; i++) 
    sBuf[i] = decode_7seg(getDigit(timeBuff, DISP_DIGITS - 1 - i) + '0');

  // Set the decimal place if the second is even
  if (now.second() % 2 == 0) sBuf[1] = sBuf[1] | 0b00000001;
}

void screenUpdate() {
  for (int i = 0; i < DISP_DIGITS; i++) digitalWrite(disp_dig_pins[i], digit != i);

  writeDigit(sBuf[digit]);

  if (digit == DISP_DIGITS - 1 && !actLEDSt) sBuf[digit] = sBuf[digit] | 0b00000001;
  else if (digit == DISP_DIGITS - 1 && actLEDSt) sBuf[digit] = sBuf[digit] & 0b11111110;
   
  digit++;
  uCyc ++;

  if (uCyc == REF_RATE / 5) {
    // Update digits
    switch (scnOp) {
      case -1: updateScnBuffWithText(tBuf); break;
      case 0: updateScnBuffWithData(uVal); break;
      case 1: updateScnBuffWithTime(); break;
      case 2: dispSleep(); break;
    }

    actLEDSt = !actLEDSt;
    
    // Toggle heartbeat LED
    digitalWrite(72, actLEDSt);

    uCyc = 0;
  }

  if (digit >= DISP_DIGITS) digit = 0;
}
