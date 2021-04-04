/* ###### Variables ###### */
// Pins
const uint8_t disp_seg_pins[] PROGMEM = { 22, 25, 26, 29, 30, 33, 34, 37 };
const uint8_t disp_dig_pins[] PROGMEM = { 50, 51, 52, 53 };

// Display vars
uint32_t digit = 0;
uint16_t uCyc  = 0;
uint16_t uVal  = 0; // New val for screen waiting to be updated
volatile int8_t scnOp = -1; // -1 - Show scrolling text; 0 - Update values;
int8_t nScnOp  = -1;
String   tBuf  = String();
uint8_t sBuf[] = {0x00, 0x00, 0x00, 0x00};
uint8_t errState = 0; // 0 = no error
bool actLEDSt  = true;
volatile long ignoreBtn = 0;

// Objects
RTC_DS1307 rtc; // RTC object
/* ############ */

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

/* ###### Helper functions ###### */
uint8_t decode_7seg(unsigned char chr) {
  if (chr > (unsigned char)'z' || chr < (unsigned char)'0') return 0x00;
  return seven_seg_digits_decode_abcdefg[chr - '0'] << 1;
}
void addDp(uint8_t digit) {
  sBuf[digit] = sBuf[digit] | 0b00000001; // Add decimal point dot
}
void updateFromDecimal(uint16_t val) {
  for (int i = 0; i < DISP_DIGITS; i++) 
    sBuf[i] = decode_7seg(getDigit(val, DISP_DIGITS - 1 - i) + '0');
}
/* ############ */

// Value setter functions

void updateDataBuff(uint16_t value) {
  uVal = value;
}

void setMode(int8_t nMode, bool persist = true) {
  if (scnOp >= MAX_MODE || scnOp < -1) scnOp = 0;
  else scnOp = nMode;
  
  if (persist && scnOp >= 0) {
    rtc.writenvram(0, scnOp); // Persistant storage for mode
    switch(scnOp) {
      case 0: tBuf = "Adc   "; break;
      case 1: tBuf = "rtc   "; break;
      case 2: tBuf = "date   "; break;
      default: tBuf ="sleep   "; break;
    }
    nScnOp = scnOp;
    SerialUSB.println(scnOp);
    setMode(-1, false);
  }
}

void changeMode() {
  if (millis() - DEBOUNCE_T < ignoreBtn) return;
  // Do whatever you want to do here
  if (scnOp + 1 >= MAX_MODE) setMode(0);
  else setMode(scnOp + 1);
  
  ignoreBtn = millis();
}

void initDisp(uint16_t ref_rate) {
  // Init i/o
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
  tBuf = "Hello   ";
  setMode(-1, false);
  
  // Get and set previous mode
  if (errState != 10 && errState != 12) nScnOp = rtc.readnvram(0);
  else nScnOp = 0;

  ignoreBtn = millis();
}

char getDigit(uint16_t value, byte digit) {
  return static_cast<uint16_t>(value / pow(10, digit)) % 10;
}

void writeDigit(uint8_t pins) {
  for (int i = 0; i < 8; i++) digitalWrite(disp_seg_pins[i], (pins >> (7 - i)) & 1);
}

/* ###### Screen buffer update functions ###### */
void updateScnBuffWithData(uint16_t value) {
  updateFromDecimal(value);
  addDp(0);
}

uint16_t txtScrollLoc = 0;
uint8_t  txtScrollInt = 0;
bool     initShowMode = true;
void updateScnBuffWithText(String txt) {
  String buff = txt.substring(txtScrollLoc, txtScrollLoc + 4);
  if (txtScrollLoc + 3 >= txt.length()) {
    txtScrollLoc = 0;
    setMode(nScnOp, initShowMode);
    initShowMode = false;
  }
  
  for (int i = 0; i < DISP_DIGITS; i++) sBuf[i] = decode_7seg(buff[i]);
  
  txtScrollInt++;
  if (txtScrollInt == 4) {
    txtScrollInt = 0;
    txtScrollLoc++;
  }
}

void updateScnBuffWithTime() {
  // Update the disp time
  DateTime now = rtc.now();

  // Then do the same thing as updateScnBuffWithDate to populate display buff
  updateFromDecimal((now.hour() * 100) + now.minute());

  // Set the decimal place if the second is even
  if (now.second() % 2 == 0) addDp(1);
}

void updateScnBuffWithDate() {
  DateTime now = rtc.now();
  updateFromDecimal((now.day() * 100) + now.month());

  addDp(1);
}

void dispSleep() {
  sBuf[0] = 0x00;
  sBuf[1] = 0x00;
  sBuf[2] = 0x00;
  sBuf[3] = 0x00;
}
/* ############ */

void screenUpdate() {
  for (int i = 0; i < DISP_DIGITS; i++) digitalWrite(disp_dig_pins[i], digit != i);

  writeDigit(sBuf[digit]);

  if (digit == DISP_DIGITS - 1 && !actLEDSt) addDp(digit);
  else if (digit == DISP_DIGITS - 1 && actLEDSt) sBuf[digit] = sBuf[digit] & 0b11111110;
   
  digit++;
  uCyc ++;

  if (uCyc == REF_RATE / 5) {
    // Update digits
    
    switch (scnOp) {
      case -1: updateScnBuffWithText(tBuf); break;
      case 0: updateScnBuffWithData(uVal); break;
      case 1: updateScnBuffWithTime(); break;
      case 2: updateScnBuffWithDate(); break;
      default: dispSleep();
    }

    actLEDSt = !actLEDSt;
    
    // Toggle heartbeat LED
    digitalWrite(72, actLEDSt);

    uCyc = 0;
  }

  if (digit >= DISP_DIGITS) digit = 0;
}
