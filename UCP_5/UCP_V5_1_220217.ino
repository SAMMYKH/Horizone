/* 
 UCP V5.1 Wireless/Multifunction
 
 02/09/14: Production Testing release. Mode is Momentary. ID = 0001 - No ID saving.
 12/09/14: Added Osc. calibration, Check IO (LED in, Pendant, Ext in). P/O Alarm is straight through-CPU does nothing.
 24/02/15: removed Cancel beep/light, changed 0.5s to 0.3s for press, put in delay after ID verify to prevent multi triggers, 
 removed boundaries Call ID= 0000-3FFF, 
 04/05/15: Added the 3 Modes, Added File date (version) storage, 
 changed ID address to loc 24 & 25,
 13/05/15: Updated with RRU 5x Memory locations and adjustments for user interfacing
 14/05/15: Added Dual Relay personality ONLY.
 05/06/15: Added Momentary, Latched & Dual Relay personalities selectable via jumpers. see below
 09/06/15: Bedlight delay = 0.5s, All calls exit delay = 0.75s, Fixed prog cancel press across modes, 
 Spare Mode setting will default as Dual Relay.
 22/02/2017: Fixed EEPROM corruption
 
 Mode (Emulating Profiles) setup:
 Momentary  => Mode1 = Open(High) & Mode2 = Open(High)
 Latch      => Mode1 = Short(Low) & Mode2 = Open(High)
 Dual Relay => Mode1 = Open(High) & Mode2 = Short(Low)
 Spare      => Mode1 = Short(Low) & Mode2 = Short(Low) - not used - will default as Dual
 */
//____________________________________________________________________________________________
#define F_CPU 8000000UL                     // Set Clk to 8MHz

#include <crc16.h>                          // for VW
#include <VirtualWire.h>                    // Using V1.19
#include <EEPROM.h>

const int callIn = 8;                       // PB0 Call button press (UCP)
const int cancelIn = 9;                     // PB1 Cancel button press (UCP)
const int mode1 = 14;                       // PC0 Profile settings
const int mode2 = 15;                       // PC1 Profile settings
const int rfData = 16;                      // PC2 RF data
const int ledIn = 17;                       // PC3 LED trigger Input (UCP)
const int extIn = 18;                       // PC4 External Input (UCP)
const int pendIn = 19;                      // PC5 Pendant button press (UCP)
const int poAlarm = 2;                      // PD2 Pendant Out Alarm Input
const int callOut = 3;                      // PD3 Call Output (UCP)
const int cancelOut = 4;                    // PD4 Cancel/Zone/Dome Output (UCP)
const int ledBright = 5;                    // PD5 UCP LEDs go bright pin (Red LED SMD on PCB) (UCP)
const int beep = 6;                         // PD6 Beeper
const int bliteOut = 7;                     // PD7 RF Bed light Output (UCP)

int modeset1;                               // Mode Mom/Latch/Dual
int modeset2;                               // Mode Mom/Latch/Dual
int mode;                                   // Which mode
int OLDMODE;
int unsigned rfIDnum;                       // Received ID number
char CharMsg[2];                            // RF Transmission container
boolean whichButton;                        // Call (true) or Bed light (false) press from Tx

int idHigh;                                 // High byte of ID num for EEPROM
int idLow;                                  // Low byte of ID num for EEPROM

int memPointer;                             // Memory location count
int memlocH;                                // Memory location High
int memlocL;                                // Memory location Low

int unsigned id1;                           // Copy of ID in loc1
int unsigned id2;                           // Copy of ID in loc2
int unsigned id3;                           // Copy of ID in loc3
int unsigned id4;                           // Copy of ID in loc4
int unsigned id5;                           // Copy of ID in loc5

int unsigned loc1H = 24;                    // Mem. location 1 High byte
int unsigned loc1L = 25;                    // Mem. location 1 Low byte
int unsigned loc2H = 26;                    //  "       "    2 High "
int unsigned loc2L = 27;                    //  "       "    2 Low "
int unsigned loc3H = 28;                    //  "       "    3 High "
int unsigned loc3L = 29;                    //  "       "    3 Low "
int unsigned loc4H = 30;                    //  "       "    4 High "
int unsigned loc4L = 31;                    //  "       "    4 Low "
int unsigned loc5H = 32;                    //  "       "    5 High "
int unsigned loc5L = 33;                    //  "       "    5 Low "

// Button status
int current;                                // Current state of the button
int count = 0;                              // How long the button was held (secs)
byte previous = LOW;                        // Idle state
int value1;                                 // used for de-bounce status
int value2;                                 // if rf call
int cancelstate;							// cancel status
//____________________________________________________________________________________________
void setup() {
  pinMode(callIn, INPUT);                   // sets the digital pin as input
  pinMode(cancelIn, INPUT);                 // sets the digital pin as input
  pinMode(mode1, INPUT);                    // sets the digital pin as input
  pinMode(mode2, INPUT);                    // sets the digital pin as input
  pinMode(rfData, INPUT);                   // sets the digital pin as input
  pinMode(ledIn, INPUT);                    // sets the digital pin as input
  pinMode(pendIn, INPUT);                   // sets the digital pin as input
  pinMode(extIn, INPUT);                    // sets the digital pin as input
  pinMode(poAlarm, INPUT);                  // sets the digital pin as input

  digitalWrite(mode1, HIGH);                // Internal Pull-up.
  digitalWrite(mode2, HIGH);                // Internal Pull-up.

  pinMode(bliteOut, OUTPUT);                // sets the digital pin as output
  pinMode(cancelOut, OUTPUT);               // sets the digital pin as output
  pinMode(callOut, OUTPUT);                 // sets the digital pin as output
  pinMode(ledBright, OUTPUT);               // sets the digital pin as output
  pinMode(beep, OUTPUT);                    // sets the digital pin as output

  // Store OSCALL into EEPROM
  if(EEPROM.read(16) == 0xFF)
    EEPROM.write(16, OSCCAL);                 // Store original osccal in addr 16
  OSCCAL = EEPROM.read(17);                 // read new fron addr 17
  if(EEPROM.read(18) != OSCCAL)
    EEPROM.write(18, OSCCAL);                 // write new osccal value to addr 18

  if(EEPROM.read(21) != 0x16)
    EEPROM.write(21, 0x16);                   // F/w date - day
  if(EEPROM.read(22) != 0x02)
    EEPROM.write(22, 0x02);                   // F/w date - month
  if(EEPROM.read(23) != 0x11)
    EEPROM.write(23, 0x11);                   // F/w date - year

  // VirtualWire - Initialise the IO and ISR
  vw_setup(2400);                           // Bits per sec
  vw_set_rx_pin(rfData);                    // Set RX pin to PC2(arduino pin 16).

  idleOut();                                // Set all O/p to Low - Idle
  readID5();                                // Copy IDs
  modeCheck();                              // Which profile am I.
  ledBlink(3, 75, beep);                    // Power on beeps
}
//____________________________________________________________________________________________
// Main program
void loop() {
  checkRf();                                // Any Rf TXs
  checkCall();                              // Call button check
  checkCancel();                            // Cancel button check
  checkIO();                                // Pendant, LED in, Ext In
  checkProg();                              // Check for Program mode
}
//____________________________________________________________________________________________
void checkCall() {                          // Check Call press
  swbounce(callIn);                         // De-bounce
  if (value1 == HIGH) {                   // if High, Call has been pressed
    switch (mode) {
    case 1:                             // Momentary routines
      momCall();
      break;
    case 2:                             // Latched routines
      latchCall();
      break;
    case 3:                             // Dual Relay routines
      dualCall();
      break;
    }
    value1 = LOW;                           // Reset ID recognition
  }
}

void checkCancel() {                        // Check Cancel press
  swbounce(cancelIn);                       // De-bounce
  if (value1 == HIGH) {                   // if High, Cancel has been pressed
    switch (mode) {
    case 1:                             // Momentary routines
      momCancel();
      break;
    case 2:                             // Latched routines
      latchCancel();
      break;
    case 3:                             // Dual Relay routines
      dualCancel();
      break;
    }
  }
}
//____________________________________________________________________________________________
void checkRf() {                            // Read Rf input buffer
  vw_rx_start();                            // Start the receiver PLL running
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  if (vw_get_message(buf, &buflen)){        // Non-blocking
    int i;
    for (i = 0; i < buflen; i++) {          // Message with a good checksum received, dump it. 
      CharMsg[i] = char(buf[i]);            // Fill CharMsg Char array with corresponding chars from buffer. 
    }
    CharMsg[buflen] = '\0';                 // Null terminate the char array. Done to stop problems when the incoming messages has less digits than the one before. 
    rfIDnum = atoi(CharMsg);                // Convert CharMsg Char array to integer 
    for (int i = 0; i < buflen; i++){       // Empty Rf buffer
      CharMsg[i] = 0;
    }
    vw_rx_stop();                             // Stop the receiver PLL running
    if ((rfIDnum >= 0 && rfIDnum <= 16383) || (rfIDnum >= 32768 && rfIDnum <= 65535)) {  // Check if its a Call, Mat or AUX number
      whichButton = true;                   // remember its a Call
    }
    else if (rfIDnum >= 16384 && rfIDnum <= 32767){ // Check if its a Bed-light number
      rfIDnum = rfIDnum & 16383;            // Clear MSB to convert back to Call ID number
      whichButton = false;                  // remember its a Bed-light
    }
    if (rfIDnum == id1 ||
      rfIDnum == id2 ||
      rfIDnum == id3 ||
      rfIDnum == id4 ||
      rfIDnum == id5 ) {                  // Check if any match rfIDnum
      switch (whichButton){
      case true:
        switch (mode) {
        case 1:                         // Momentary routines
          momCall();
          break;
        case 2:                         // Latched routines
          latchCall();
          break;
        case 3:                         // Dual Relay routines
          dualPendCall();
          break;
        }
        break;
      case false:
        digitalWrite(bliteOut, HIGH);
        delay(500);                       // 0.5sec wait
        digitalWrite(bliteOut, LOW);
        delay(250);                       // 0.25sec wait
        break;
      }
    }
    delay(500);                             // Wait so there is only one trigger.
  }
}
//____________________________________________________________________________________________
void checkProg() {                          // Check if Program mode
  // Read Program button status 'Cancel'
  swbounce(cancelIn);                       // De-bounce
  current = value1;
  if (current == HIGH && previous == LOW) { // first press
    digitalWrite(ledBright, HIGH);          // Bright LED
    digitalWrite(cancelOut, LOW);           // Idle
    digitalWrite(callOut, LOW);             // Idle 
    delay(500);                             // 0.5sec wait
    digitalWrite(ledBright, LOW);           // Idle
    count = 1;  
    delay(500);                             // 0.5sec wait
  }  
  if (current == HIGH && previous == HIGH && count < 11) {  // 2nd press to 11th press
    digitalWrite(ledBright, HIGH);          // Bright LED
    delay(500);                             // 0.5sec wait
    digitalWrite(ledBright, LOW);           // Idle
    delay(500);                             // 0.5sec wait
    count++;
  }
  // Program Mode - Stores rfIDnum RF ID number transmitted.
  if (current == LOW && previous == HIGH && count == 5){    
    delay(500);                             // wait 1.0sec
    digitalWrite(ledBright, HIGH);          // Bright LED
    delay(2000);                            // 0.5sec wait
    digitalWrite(ledBright, LOW);           // Idle
    ledBlink(1, 500, beep);                 // 0.3sec beep
    delay(500);                             // wait 1.0sec
    vw_rx_start();                          // Start the receiver PLL running
    uint8_t buf[VW_MAX_MESSAGE_LEN];
    uint8_t buflen = VW_MAX_MESSAGE_LEN;
    if (vw_get_message(buf, &buflen)){    // Non-blocking
      for (int i = 0; i < buflen; i++){   // Message with a good checksum received, dump it. 
        CharMsg[i] = char(buf[i]);        // Fill CharMsg Char array with corresponding chars from buffer. 
      }
      CharMsg[buflen] = '\0'; 
      rfIDnum = atoi(CharMsg);            // Convert CharMsg Char array to integer 
      for (int i = 0; i < buflen; i++){   // Empty rf buffer
        CharMsg[i] = 0;
      }
    }
    vw_rx_stop();                           // Stop the receiver PLL running
    if (rfIDnum != 0x0000 &&                // if not 0000 then store
    rfIDnum != id1 &&
      rfIDnum != id2 &&
      rfIDnum != id3 &&
      rfIDnum != id4 &&
      rfIDnum != id5 ){                     // Check if any match rfIDnum
      storeID5();                         // Store rfIDnum into a mem. location
      delay(500);
      readID5();                          // Reload IDs
      rfIDnum = 0x0000;                   // reset ID number
    }
  }    
  // Reset stored rfIDnum to FFFF.
  if (current == LOW && previous == HIGH && count == 10){  
    rfIDnum = 0xFFFF;                       // Erase ID number
    resetID5();                             // reset all 5 mem.
    readID5();                              // Reload IDs to static registers
    ledBlink(10, 100, ledBright);
    ledBlink(2, 200, beep);                 // OK done.
  }
  if (current == LOW){                      // reset the counter if the button is not pressed
    count = 0;
  }
  previous = current;                       // Save State of button
}
//____________________________________________________________________________________________
void checkIO() {                            // Check Pendant press, EXT In, LED In.
  int pendStat = digitalRead(pendIn);       // read Pendant IN status
  int ledStat = digitalRead(ledIn);         // read LED IN status
  int extStat = digitalRead(extIn);         // read EXT. IN status

  switch (mode) {
  case 1:                                   // Momentary routines
    if (pendStat == LOW){
      momCall();
    }
    if (ledStat == LOW){                    // Check LED IN status
      digitalWrite(ledBright, HIGH);        
      cancelstate = HIGH; 					// set cancel status to high
    }
    else if ( ledStat == HIGH && cancelstate == LOW){
      digitalWrite(ledBright, LOW);
    }
    break;

  case 2:                                   // Latched routines
    if ((pendStat == LOW)||(extStat == LOW)){
      latchCall();
    }
    break;

  case 3:                                   // Dual Relay routines
    if ((pendStat == LOW)||(extStat == LOW)){
      dualPendCall();
    }
    break;
  }
}
//____________________________________________________________________________________________
void modeCheck(){
  OLDMODE = EEPROM.read(20);
  modeset1 = digitalRead(mode1);            // read settings
  modeset2 = digitalRead(mode2);
  if (modeset1 == HIGH && modeset2 == HIGH){
    mode = 1;                               // set as Momentary
  }
  if (modeset1 == LOW && modeset2 == HIGH){
    mode = 2;                               // set as Latch
  }
  if (modeset1 == HIGH && modeset2 == LOW){
    mode = 3;                               // set as Dual Relay.
  }
  if (modeset1 == LOW && modeset2 == LOW){  // Not used
    mode = 3;                               // set as Dual Relay.
  }
  if(OLDMODE != mode)
    EEPROM.write(20, mode);                   // show Mode in EEPROM
}
//____________________________________________________________________________________________
void swbounce (int switchPin) {             // De-bounce button press
  int val = digitalRead(switchPin);         // read input value and store it in val
  delay(10);                                // 10 milliseconds is a good amount of time
  int val2 = digitalRead(switchPin);        // read the input again to check for bounces
  if ((val == val2) && (val == HIGH)) {     // make sure we got 2 consistent readings and High
    value1 = HIGH;                          // Set if is a legit press
  }
  else { 
    value1 = LOW;	                    // False alarm	
  }
}

void ledBlink(int times, int lengthms, int pinnum){	// Routine for blinking a LED
  for (int x=0; x<times;x++){
    digitalWrite(pinnum, HIGH);
    delay (lengthms);
    digitalWrite(pinnum, LOW);
    delay(lengthms);
  }
}

void idleOut() {                            // Set output states to Idle
  digitalWrite(callOut, LOW);
  digitalWrite(cancelOut, LOW);
  digitalWrite(ledBright, LOW);	
  digitalWrite(bliteOut, LOW);	
}
//____________________________________________________________________________________________
void momCall() {
  digitalWrite(callOut, HIGH);              // Set Call O/p high - triggered state
  digitalWrite(beep, HIGH);                 // Beeper on
  digitalWrite(ledBright, HIGH);            // Bright LEDs on
  digitalWrite(cancelOut, HIGH);			// Set Cancel O/p high - triggered state
  delay(500);                               // 0.5sec wait
  digitalWrite(ledBright, LOW);             // Bright LEDs off
  digitalWrite(beep, LOW);                  // Beeper off
  digitalWrite(callOut, LOW);               // Set Call O/p low - Idle state
  delay(750);                               // 0.75 sec wait
}

void latchCall() {
  digitalWrite(cancelOut, HIGH);            // K2 (Cancel) - latched
  digitalWrite(beep, HIGH);                 // Beeper on
  digitalWrite(callOut, HIGH);              // K1 is Momentary output
  digitalWrite(ledBright, HIGH);            // Bright LEDs - latched
  delay(500);                               // 0.5sec wait
  digitalWrite(callOut, LOW);               // 
  digitalWrite(beep, LOW);                  // Beeper off
  delay(750);                               // 0.75sec wait  
}  

void dualCall() {
  digitalWrite(callOut, HIGH);              // K1 Dome (Call) - latched
  digitalWrite(beep, HIGH);                 // Beeper on
  digitalWrite(ledBright, HIGH);            // Bright LEDs - latched
  delay(500);                               // 0.5sec wait
  digitalWrite(beep, LOW);                  // Beeper off
  delay(750);                               // 0.75sec wait
}

void dualPendCall() {
  digitalWrite(cancelOut, HIGH);            // K2 Zone (Cancel) - latched
  digitalWrite(beep, HIGH);                 // Beeper on
  digitalWrite(ledBright, HIGH);            // Bright LEDs - latched
  delay(500);                               // 0.5sec wait
  digitalWrite(beep, LOW);                  // Beeper off
  delay(750);                               // 0.75sec wait
}
//____________________________________________________________________________________________
void momCancel() {
  digitalWrite(cancelOut, LOW);             // Set Cancel O/p LOW - Idle state
  cancelstate = LOW;						// set cancel status LOW
  delay(300);                               // 0.3sec wait
  digitalWrite(ledBright, LOW);             // Bright LEDs off
  delay(250);                               // 0.25sec wait
  value1 = LOW;                             // reset sw press ok
}

void latchCancel() {
  digitalWrite(ledBright, LOW);             // Bright LEDs off
  digitalWrite(cancelOut, LOW);             // Set Call O/p low - Idle state
  delay(250);                               // 0.25sec wait
}

void dualCancel() {
  digitalWrite(ledBright, LOW);             // Bright LEDs off
  digitalWrite(callOut, LOW);               // K1 Dome OFF - Idle state
  digitalWrite(cancelOut, LOW);             // K2 Zone OFF - Idle state
  delay(250);                               // 0.25sec wait
}
//____________________________________________________________________________________________
void storeID5() {                           // Stores ID no. in EEPROM for 5 locations
  memPointer = EEPROM.read(19);             // Read pointer from addr 19
  if (memPointer >= 6) {                    // Are memory locations full
    delay(5000);                            // OFF for 5 sec.
    EEPROM.write(19, memPointer);           // Store memPointer in addr 19  
  }
  else if (memPointer == 1) {             // Loc. 1
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc1H, idHigh);          // Store high in addr 24
    EEPROM.write(loc1H +1, idLow);        // Store low in addr 25
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, beep);               // OK done.
  }
  else if (memPointer == 2) {             // Loc. 2
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc2H, idHigh);          // Store high in addr 26
    EEPROM.write(loc2H +1, idLow);        // Store low in addr 27
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, beep);               // OK done.
  }
  else if (memPointer == 3) {             // Loc. 3
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc3H, idHigh);          // Store high in addr 28
    EEPROM.write(loc3H +1, idLow);        // Store low in addr 29
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, beep);               // OK done.
  }
  else if (memPointer == 4) {             // Loc. 4
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc4H, idHigh);          // Store high in addr 30
    EEPROM.write(loc4H +1, idLow);        // Store low in addr 31
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, beep);               // OK done.
  }
  else if (memPointer == 5) {             // Loc. 5
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc5H, idHigh);          // Store high in addr 32
    EEPROM.write(loc5H +1, idLow);        // Store low in addr 33
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, beep);               // OK done.
  }
}

void resetID5() {                           // Resets ID no. in EEPROM
  idLow = lowByte(rfIDnum);                 // break up into 2 bytes
  idHigh = highByte(rfIDnum);
  EEPROM.write(loc1L, idLow);               // Store low in addr 25
  EEPROM.write(loc1H, idHigh);              // Store high in addr 24
  EEPROM.write(loc2L, idLow);               // Store low in addr 27
  EEPROM.write(loc2H, idHigh);              // Store high in addr 26
  EEPROM.write(loc3L, idLow);               // Store low in addr 29
  EEPROM.write(loc3H, idHigh);              // Store high in addr 28
  EEPROM.write(loc4L, idLow);               // Store low in addr 31
  EEPROM.write(loc4H, idHigh);              // Store high in addr 30
  EEPROM.write(loc5L, idLow);               // Store low in addr 33
  EEPROM.write(loc5H, idHigh);              // Store high in addr 32
  memPointer = 1;                           // reset mem pointer
  EEPROM.write(19, memPointer);             // Store memPointer in addr 19    
}

void readID5() {                            // Reads ID no. x5 from EEPROM
  idLow = EEPROM.read(loc1L);               // Read low from addr 25
  idHigh = EEPROM.read(loc1H);              // Read high from addr 24
  id1 = word(idHigh, idLow);                // convert to one word

  idLow = EEPROM.read(loc2L);               // Read low from addr 27
  idHigh = EEPROM.read(loc2H);              // Read high from addr 26
  id2 = word(idHigh, idLow);                // convert to one word

  idLow = EEPROM.read(loc3L);               // Read low from addr 29
  idHigh = EEPROM.read(loc3H);              // Read high from addr 28
  id3 = word(idHigh, idLow);                // convert to one word

  idLow = EEPROM.read(loc4L);               // Read low from addr 31
  idHigh = EEPROM.read(loc4H);              // Read high from addr 30
  id4 = word(idHigh, idLow);                // convert to one word

  idLow = EEPROM.read(loc5L);               // Read low from addr 33
  idHigh = EEPROM.read(loc5H);              // Read high from addr 32
  id5 = word(idHigh, idLow);                // convert to one word
}



