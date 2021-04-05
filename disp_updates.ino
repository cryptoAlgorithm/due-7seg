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
  if (txtScrollInt == 3) {
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

uint8_t scnCntM = 0;
uint8_t scnCnt  = BONG_MAX_T;
void updateScnBuffWithCount() {
  scnCntM++;
  if (((scnCnt <= 3 && scnCntM == 3) || (scnCntM >= 6)) && scnCnt > 0) {
    scnCnt--;
    scnCntM = 0;
  }
  if (scnCnt <= 0 && scnCntM <= 40) {
    sBuf[0] = decode_7seg('b');
    sBuf[1] = decode_7seg('o');
    sBuf[2] = decode_7seg('m');
    sBuf[3] = decode_7seg('b');
  }
  else if (scnCnt <= 0 && scnCntM > 20) {
    scnCnt = BONG_MAX_T;
    scnCntM = 0;
  }
  else updateFromDecimal(scnCnt);
}

void dispSleep() {
  sBuf[0] = 0x00;
  sBuf[1] = 0x00;
  sBuf[2] = 0x00;
  sBuf[3] = 0x00;
}
/* ############ */
