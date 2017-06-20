/*  
 RRU Receiver with ATTiny85
 
 24/04/15: Copied from 'Rf_Dongle_V1_2_Apr23', Added File date (version) storage, 
 changed ID address to loc 24 & 25, Externally Fuses (BOD=4.7V) [FF D4 E2],
 Split delays in loop, turned PLL Off/On when needed.
 29/04/15: Fixed up Dim & Bright LED routines, Set ID to 0001 @ start up, 
 06/05/15: Added 5 ID memory locations Read & Compare, cleaned up code.
 11/05/15: Added 5 ID memory location Write & Pointer.  
 12/05/15: User tweaks-cleaned up store5ID, prog. delays 
 13/05/15: Programming errors fixed.
 22/02/2017: Fixed EEPROM corruption
 */

//____________________________________________________________________________________________
#define F_CPU 8000000UL                     // Set Clk to 8MHz

#include <crc16.h>                          // for VW
#include <VirtualWire.h>                    // Using V1.19
#include <EEPROM.h>

const int LED = 0;                          // PB0 - Pin5 LED
const int progButton = 1;                   // PB1 - Pin6 Switch
const int rfData = 2;                       // PB2 - Pin7 RF data
const int Bed = 3;                          // PB3 - Pin2 BedLight
const int Call = 4;                         // PB4 - Pin3 Call

//int unsigned myIDnum;                       // Initial ID number
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

//____________________________________________________________________________________________
void setup() {
  pinMode(LED, OUTPUT);                     // LED is output
  digitalWrite (LED, LOW);                  // turn off LED
  pinMode(LED, INPUT);                      // In tristate for power on LED
  pinMode(progButton,INPUT);                // progButton is input
  pinMode(rfData, INPUT);                   // rfData is input
  pinMode(Bed, OUTPUT);                     // bedLight is output
  digitalWrite (Bed, LOW);                  // turn off bedLight
  pinMode(Call, OUTPUT);                    // CALL is output
  digitalWrite (Call, LOW);                 // turn off Call

  // Store OSCALL into EEPROM
 // if(EEPROM.read(16) == 0xFF)                //to avoid unnecessary write function on EEPROM 
    EEPROM.write(16, OSCCAL);                 // Store original osccal in addr 16
  OSCCAL = EEPROM.read(17);                 // read new fron addr 17
  if(EEPROM.read(18) != OSCCAL)              //to avoid unnecessary write function on EEPROM 
    EEPROM.write(18, OSCCAL);                 // write new osccal value to addr 18

  if(EEPROM.read(21) != 0x16)                //to avoid unnecessary write function on EEPROM  
    EEPROM.write(21, 0x16);                       // F/w date - 22 day
  if(EEPROM.read(22) != 0x02)                //to avoid unnecessary write function on EEPROM 
    EEPROM.write(22, 0x02);                       // F/w date - 02 month
  if(EEPROM.read(23) != 0x11)                //to avoid unnecessary write function on EEPROM 
    EEPROM.write(23, 0x11);                       // F/w date - 2017 year

  // VirtualWire - Initialise the IO and ISR
  vw_setup(2400);                           // Bits per sec (was 2400)
  vw_set_rx_pin(rfData);                    // Set RX pin to PB2(arduino pin 7).

  readID5();                                // Copy IDs
  ledBlink(3, 75, LED);                     // Power on flashes
}

//____________________________________________________________________________________________
// Main program
void loop() {
  checkRf();                                // check Rf
  delay (100);
  checkProg();                              // check Program button
  delay (100);
}

//____________________________________________________________________________________________
void checkProg() {                          // Check Program button press
  swbounce(progButton);                     // De-bounce
  current = value1;

  if (current == HIGH && previous == LOW) { // first press
    ledBlink(1, 400, LED);
    count = 1;  
  }
  if (current == HIGH && previous == HIGH && count < 11) {  // 2nd press to 11th press
    ledBlink(1, 400, LED);
    count++;
  }
  if (current == LOW && previous == HIGH && count == 5){    // Program Mode - Stores rfIDnum RF ID number transmitted.
    delay(1000);

    pinMode(LED, OUTPUT);                   // Enable LED line
    //    digitalWrite(LED, LOW);                 // Turn off LED
    //    delay(200);
    digitalWrite(LED, HIGH);
    delay(500);

    //    ledBlink(1, 200, LED);
    //    pinMode(LED, OUTPUT);                   // Enable LED line
    digitalWrite(LED, LOW);                 // Turn off LED
    delay(1000);
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
  if (current == LOW && previous == HIGH && count == 10){  // Reset stored rfIDnum to FFFF.
    rfIDnum = 0xFFFF;                       // Erase ID number
    resetID5();                             // reset all 5 mem.
    readID5();                              // Reload IDs to static registers
    ledBlink(10, 100, LED);
  }
  if (current == LOW){                      // reset the counter if the button is not pressed
    count = 0;
  }
  previous = current;                       // Save State of button
  pinMode(LED, INPUT);                      // In tristate for Idle LED
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
      rfIDnum == id5 ){                   // Check if any match rfIDnum

      switch (whichButton){
      case true:
        value2 = HIGH;                      // Set - it is a RF Call
        digitalWrite(Call, HIGH);
        ledBlinkhalf(1, 500, LED);
        digitalWrite(Call, LOW);
        break;
      case false:
        digitalWrite(Bed, HIGH);
        ledBlinkhalf(1, 500, LED);
        digitalWrite(Bed, LOW);
        break;
      }
    }
    delay(500);                               // Wait so there is only one trigger.
  }
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

void ledBlink(int times, int lengthms, int pinnum){  // Routine for blinking a pin
  pinMode(LED, OUTPUT);                     // Enable LED line
  digitalWrite(pinnum, LOW);
  //  delay(lengthms);
  for (int x=0; x<times;x++){
    digitalWrite(pinnum, HIGH);
    delay (lengthms);
    digitalWrite(pinnum, LOW);
    delay(lengthms);
  }
  pinMode(LED, INPUT);                      // In tristate for Idle LED
}

void ledBlinkhalf(int times, int lengthms, int pinnum){  // Routine for blinking a pin
  pinMode(LED, OUTPUT);                     // Enable LED line
  for (int x=0; x<times;x++){
    digitalWrite(pinnum, HIGH);
    delay (lengthms);
    digitalWrite(pinnum, LOW);
  }
  pinMode(LED, INPUT);                      // In tristate for power on LED
}

/*void storeID() {                            // Stores ID no. in EEPROM
 	idLow = lowByte(rfIDnum);                 // break up into 2 bytes
 	idHigh = highByte(rfIDnum);
 	EEPROM.write(loc1L, idLow);               // Store low in addr 25
 	EEPROM.write(loc1H, idHigh);              // Store high in addr 24
 }
 */
void storeID5() {                           // Stores ID no. in EEPROM for 5 locations
  memPointer = EEPROM.read(19);             // Read pointer from addr 19
  if (memPointer >= 6) {                    // Are memory locations full
    pinMode(LED, OUTPUT);                   // Enable LED line
    digitalWrite(LED, LOW);
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
    ledBlink(2, 200, LED);                // OK done.
  }
  else if (memPointer == 2) {             // Loc. 2
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc2H, idHigh);          // Store high in addr 26
    EEPROM.write(loc2H +1, idLow);        // Store low in addr 27
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, LED);                // OK done.
  }
  else if (memPointer == 3) {             // Loc. 3
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc3H, idHigh);          // Store high in addr 28
    EEPROM.write(loc3H +1, idLow);        // Store low in addr 29
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, LED);                // OK done.
  }
  else if (memPointer == 4) {             // Loc. 4
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc4H, idHigh);          // Store high in addr 30
    EEPROM.write(loc4H +1, idLow);        // Store low in addr 31
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, LED);                // OK done.
  }
  else if (memPointer == 5) {             // Loc. 5
    idLow = lowByte(rfIDnum);             // break up into 2 bytes
    idHigh = highByte(rfIDnum);
    EEPROM.write(loc5H, idHigh);          // Store high in addr 32
    EEPROM.write(loc5H +1, idLow);        // Store low in addr 33
    memPointer ++;                        // increment Memory pointer
    EEPROM.write(19, memPointer);         // Store memPointer in addr 19      
    ledBlink(2, 200, LED);                // OK done.
  }
}

void resetID5() {                           // Stores ID no. in EEPROM
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

/*void readID() {                             // Reads ID no. from EEPROM
 	idLow = EEPROM.read(loc1L);               // Read low from addr 25
 	idHigh = EEPROM.read(loc1H);              // Read high from addr 24
 	rfIDnum = word(idHigh, idLow);            // convert to one word
 }
 */
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

