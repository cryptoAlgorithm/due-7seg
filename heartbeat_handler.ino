uint16_t hbVal = 5;
bool hbDir = true; // true for add, false for subtract
uint8_t hbSkipCyc = 0; // Skip certain update cycles

void hbInit() {
  // Setup hardware timer
  Timer7.attachInterrupt(hbHandler).start(50000);
}

void hbHandler() {
  if (hbSkipCyc > 0) {
    hbSkipCyc--;
    return;
  }
  
  analogWrite(LED_BUILTIN, hbVal);
  
  if (hbDir) hbVal += 5;
  else hbVal -= 5;
  
  if (hbVal >= 255 || hbVal <= 0) {
    hbDir = !hbDir;
    hbSkipCyc = 20;
  }
}
